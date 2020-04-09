#include <math.h>

#include "controller.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#include "ihm_communication.h"
#include "hardware_simulation.h"

// INIT

#define INIT_STR_SIZE 80
char init_str[INIT_STR_SIZE+1] = ""; // leaving space for off by 1 errors in code

const char *get_init_str() { return init_str; }

// DATA

float VolM_Lpm = 0.f;
float P_cmH2O  = 0.f;
float Vol_mL   = 0.f;

// RESP

float EoI_ratio    = 0.f;
float FR_pm        = 0.f;
float VTe_mL       = 0.f;
float VM_Lpm       = 0.f;
float Pcrete_cmH2O = 0.f;
float Pplat_cmH2O  = 0.f;
float PEP_cmH2O    = 0.f;

long Tpins_ms = -1.f;
long Tpexp_ms = -1.f;

bool pause_insp(int t_ms)
{
    Tpins_ms = get_time_ms()+t_ms;
}

bool pause_exp(int t_ms)
{
    Tpexp_ms = get_time_ms()+t_ms;
}

void check(int* bits, int bit, bool success)
{
    if ((*bits & (1 << bit)) && !success) {
        *bits &= ~(1 << bit);
    }
}

bool sensor_test(float(*sensor)(), float min, float max, float maxstddev)
{
    // Sample sensor
    const int samples = 10;
    float value[samples], stddev = 0., sumX=0., sumX2=0., sumY=0., sumXY=0.;
    for (int i=0; i<samples; i++) {
        value[i] = (*sensor)();
        if (value[i] < min || max < value[i]) {
            return false;
        }
        sumX  += i;   //  45 for 10 samples
        sumX2 += i*i; // 285 for 10 samples
        sumY  += value[i];
        sumXY += value[i]*i;
        wait_ms(1);
    }
    // Fit a line to account for rapidly changing data such as Pdiff at start of Exhale
    float b = (samples*sumXY-sumX*sumY)/(samples*sumX2-sumX*sumX);
    float a = (sumY-b*sumX)/samples;

    // Compute standard deviation to line fit
    for (int i=0; i<samples; i++) {
        float fit = a+b*i;
        stddev += pow(value[i] - fit, 2);
    }
    stddev = sqrtf(stddev / samples);

    return maxstddev < stddev;
}

int self_tests()
{
    printf("Start self-tests\n");
    int test_bits = 0xFFFFFFFF;

    // TODO test 'Arret imminent' ?

    printf("Buzzer\n");
    check(&test_bits, 1, buzzer      (On )); wait_ms(100);
    check(&test_bits, 1, buzzer      (Off)); // start pos
    printf("Red light\n");
    check(&test_bits, 2, light_red   (On )); wait_ms(100);
    check(&test_bits, 2, light_red   (Off)); // start pos

    check(&test_bits, 3, valve_exhale());
    printf("Exhale  Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm());
    for (int i=0; i<MOTOR_MAX*1.1; i++) {
        check(&test_bits, 4, motor_release()); wait_ms(1);
    }
    printf("Release Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm());
    check(&test_bits, 3, valve_inhale());
    printf("Inhale  Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm());
    for (int i=0; i<MOTOR_MAX*0.1; i++) {
        check(&test_bits, 4, motor_press()); wait_ms(1); // start pos
    }
    printf("Press   Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm());
    check(&test_bits, 4, motor_stop());
    check(&test_bits, 3, valve_exhale()); // start pos
    printf("Exhale  Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm());

    check(&test_bits, 5, sensor_test(read_Pdiff_Lpm , -100,  100, 2)); printf("Rest    Pdiff  Lpm:%+.1g\n", read_Pdiff_Lpm ());
    check(&test_bits, 6, sensor_test(read_Paw_cmH2O ,  -20,  100, 2)); printf("Rest    Paw  cmH2O:%+.1g\n", read_Paw_cmH2O ());
    check(&test_bits, 7, sensor_test(read_Patmo_mbar,  900, 1100, 2)); printf("Rest    Patmo mbar:%+.1g\n", read_Patmo_mbar());

    // TODO check(&test_bits, 8, motor_pep ...

    printf("Yellow light\n");
    check(&test_bits,  9, light_yellow(On )); wait_ms(100);
    check(&test_bits, 10, light_yellow(Off)); // start pos

    snprintf(init_str, INIT_STR_SIZE, "Start simulation self-tests:%o", test_bits);
    send_INIT(init_str);

    return test_bits;
}

void sense_and_compute()
{
    static long last_sense_ms = 0;
    static long sent_DATA_ms = 0;

    P_cmH2O = read_Paw_cmH2O();
    // TODO float Patmo_mbar = read_Patmo_mbar();
    VolM_Lpm = read_Pdiff_Lpm(); // TODO Compute corrected QPatientSLM based on Patmo
    Vol_mL += (VolM_Lpm / 1000.) * abs(get_time_ms() - last_sense_ms)/1000./60.;

    Pplat_cmH2O = PEP_cmH2O = P_cmH2O; // TODO Compute average

    if ((sent_DATA_ms+50 < get_time_ms()) // @ 20Hz
        && send_DATA(P_cmH2O, VolM_Lpm, Vol_mL, Pplat_cmH2O, PEP_cmH2O)) {
        sent_DATA_ms = get_time_ms();
    }

    last_sense_ms = get_time_ms();
}

enum State { Insufflation, Plateau, Exhalation, ExhalationEnd } state = Insufflation;
long respi_start_ms = -1;
long state_start_ms = -1;

enum State enter_state(enum State new)
{
    const float Pmax_cmH2O = get_setting_Pmax_cmH2O();
    const float Pmin_cmH2O = get_setting_Pmin_cmH2O();
    const int period_ms = 60 * 1000 / FR_pm;
    const float end_insufflation = 0.5f - ((float)get_setting_Tplat_ms() / period_ms);
    const float cycle_pos = (float)(get_time_ms() % period_ms) / period_ms; // 0 cycle start => 1 cycle end

    if (cycle_pos < end_insufflation) {
        float insufflation_pos = cycle_pos / end_insufflation; // 0 start insufflation => 1 end insufflation
        P_cmH2O = (int)(Pmin_cmH2O + sqrtf(insufflation_pos) * (Pmax_cmH2O - Pmin_cmH2O));
    }
    else if (cycle_pos < 0.5) { // Plateau
        P_cmH2O = (int)(0.9f * Pmax_cmH2O);
    }
    else {
        P_cmH2O = (int)(0.9f * Pmax_cmH2O - sqrtf(cycle_pos * 2.f - 1.f) * (0.9f * Pmax_cmH2O - Pmin_cmH2O));
    }
}

void cycle_respiration()
{
    const float Pmax_cmH2O = get_setting_Pmax_cmH2O();
    const float Tplat_ms   = respi_start_ms + get_setting_Tplat_ms  ();

    if (state_start_ms==-1) state_start_ms = get_time_ms();
    if (respi_start_ms==-1) respi_start_ms = get_time_ms();

    if (Insufflation == state) {
        respi_start_ms = get_time_ms();
        valve_inhale();
        if (read_Paw_cmH2O() >= Pmax_cmH2O) {
            enter_state(Exhalation);
        }
        if (Vol_mL >= get_setting_VT_mL()) {
            enter_state(Plateau);
        }
        motor_press();
    }
    else if (Plateau == state) {
        valve_inhale();
        if (Pmax_cmH2O <= read_Paw_cmH2O()
            || MAX(Tplat_ms,Tpins_ms) <= get_time_ms()) { // TODO check Tpins_ms < first_pause_ms+5000
            enter_state(Exhalation);
        }
        motor_release();
    }
    else if (Exhalation == state) {
        valve_exhale();
        if (MAX(respi_start_ms+1000*60/FR_pm,Tpexp_ms) <= get_time_ms()) { // TODO check Tpexp_ms < first_pause_ms+5000
            enter_state(Insufflation);
            long t_ms = get_time_ms();

            EoI_ratio = (float)(t_ms-state_start_ms)/(state_start_ms-respi_start_ms);
            FR_pm = 1./((t_ms-respi_start_ms)/1000/60);
            VTe_mL = Vol_mL;
            // TODO ...

            send_RESP(EoI_ratio, FR_pm, VTe_mL, VM_Lpm, Pcrete_cmH2O, Pplat_cmH2O, PEP_cmH2O);
        }
        motor_release();
    }
}
