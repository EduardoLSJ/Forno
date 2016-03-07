/******************************************************************************/

//#include "interrupts.c"

/* User Level #define Macros                                                  */
/******************************************************************************/
// Definition of hardware version
#define RevA 1 //first hardware version
#define RevB 2

// Current revision is:
#define SMTOVEN  RevA
//#define SMTOVEN  RevB
//#define SMTOVEN  RevC

#define TRISA_CONF 0x00
#define TRISA_CONF_READ 0x10
#define TRISB_CONF 0x38
#define TRISC_CONF 0xC2


//LCD
//#define LCD_DAT PORTA
//#define LCD_TRIS TRISA
//#define LCD_DB4 PORTAbits.RA0
//#define LCD_DB5 PORTAbits.RA1
//#define LCD_DB6 PORTAbits.RA2
//#define LCD_DB7 PORTAbits.RA3
//#define LCD_E PORTCbits.RC4
//#define LCD_RS PORTCbits.RC5

//SPI
#define SPI_SCK PORTAbits.RA5
#define SPI_NCS PORTCbits.RC0
#define SPI_SO PORTCbits.RC1

//I2C
#define I2C_SDA1 PORTAbits.RA4
#define I2C_SDATRIS TRISAbits.TRISA4
#define I2C_SCL1 PORTAbits.RA5

//TECLADO
#define KEY_bits PORTB
#define KEY_TRIS TRISB

//TRIAC
#define HEAT_port PORTB
#define HEAT1 PORTBbits.RB0
#define HEAT2 PORTBbits.RB1

//BUZZER
#define BUZZER LATBbits.LATB2
//#define BUZZER_TRIS TRISBbits.TRISB2
//#define BUZZER_ON BUZZER_TRIS = 0;
//#define BUZZER_OFF BUZZER_TRIS = 1;

//Bluetooth
#define BT_reset LATCbits.LATC2
#define BT_CMD LATCbits.LATC3

// Timer 0 count (at 4MHz) 500us
#define RT0Prescaler 3
#define RT0Reload 150
#define T0COUNTVAL 2000
//contador para 50/sec 
#define TCOUNTVAL 40

#define NRPROFILES 10		// max nr of profiles stored in EEPROM

#define MENU_START  1
#define MENU_EDIT   2
#define MENU_LOG    3
#define MENU_CAL    4
#define MENU_MAX    4
#define MENU_MIN    1

typedef enum EVENT_edit {
    ev_none,
    ev_enter,
    ev_right,
    ev_left
};

typedef enum STATE_edit {
    st_editar,
    st_pre_slope,
    st_pre_endtemp,
    st_pre_time,
    st_soak_time,
    st_soak_endtemp,
    st_reflow_time,
    st_reflow_peaktemp
};

enum max_value_list { //A4
    max_editar = 0,//stuffing
    max_pre_slope = 5,
    max_pre_endtemp = 200,
    max_pre_time = 600,
    max_soak_time = 600,
    max_soak_endtemp = 200,
    max_reflow_time = 60,
    max_reflow_peaktemp = 300
};

enum min_value_list { //A4
    min_editar = 0,//stuffing
    min_pre_slope = 1,
    min_pre_endtemp = 75,
    min_pre_time = 1,
    min_soak_time = 10,
    min_soak_endtemp = 75,
    min_reflow_time = 1,
    min_reflow_peaktemp = 150
};

enum mensagens_main_list {
  Forno,
  Forno1,
  Inicializando,
  Forno_SMD,
  FALHA,
  troque,
  Calibracao,
  ENT,
  DINST,
  Iniciar,
  Editar,
  Registrar,
  Calibrar,
  Terminado,
  press,
  DINST1
};

const char* mensagens_main[] = {
  "Forno SMD v1.0\n\r",
  "Forno SMD v1.0",
  "  Inicializando",
  "\n\r\n\rForno SMD\n\r",
  "FALHA DA EEPROM!",
  "troque o 24c1024",
  "Calibracao",
  "[ENT] inicia",
  "DINST   SMD-OVEN",
  "->Iniciar Solda",
  "->Editar Perfil",
  "->Registrar",
  "->Calibrar",
  "Terminado",
  "press [ENT]",
  "DINST     %2uh%2um"
};

enum mensagens_calibrate_list {
    ESFRIANDO,
    AQUECENDO,
    SOBRETEMP,
    PRONTO
};

const char* mensagens_calibrate[] = {
    "CAL : ESFRIANDO",
    "CAL : AQUECENDO",
    "CAL : SOBRETEMP",
    " CAL : PRONTO"
};

enum mensagens_controle_list {
    PRE_RES,
    SOAK,
    DWELL,
    REFLUXO,
    RESFRIANDO
};

const char* mensagens_controle[] = {
    "PRE-RESFRIAMENTO",
    "SOAK",
    "DWELL",
    "REFLUXO",
    "RESFRIANDO"
};

enum mensagens_edit_list {
    Salva,
    sim
};

const char* mensagens_edit[] = {
    "Salva ajustes?",
    "[<] n  /  s [>]"
};

enum mensagens_send_log_list {
    Enviando,
    velocidade
};

const char* mensagens_send_log[] = {
    "Enviando tabela",
    "   9600,8,N,1"
};

enum mensagens_logging_list {
    tecle,
    Reg_on,
    Reg_off
};

const char* mensagens_logging[] = {
    "tecle [>] enviar",
    "Registro : ON",
    "Registro : OFF"
};

const char *str_line_0[] = {
    "EDITAR  <SAIR>",
    "EDITAR : pre-aqu",
    "EDITAR : pre-aqu",
    "EDITAR : pre-aqu",
    "EDITAR : soak",
    "EDITAR : soak",
    "EDITAR : reflow",
    "EDITAR : reflow"
};

const char *str_line_1[] = {
    "               ", //EDITAR
    "slope:%2u\xDF C/s",//PRE_SLOPE
    "Temp : %3u\xDF C",// PRE_ENDTEMP
    "Interv  %2.2u:%2.2u",//PRE_TIME
    "Interv  %2.2u:%2.2u",// SOAK_TIME
    "TempFim : %3u\xDF C",// SOAK_ENDTEMP
    "Interv : %2u seg",// REFLOW_TIME
    "temp : %3u\xDF C",// REFLOW_ENDTEMP
};

union Uperfis {

    struct SizeProfile {
        unsigned int stuffing_editar;
        unsigned int pre_slope; // max slope in the preheat phase in 0.2 C / sec.
        //Reccomm. is 2C/sec to 3C/sec
        unsigned int pre_endtemp; // end temp in the preheat phase in 1 C.
        //Reccomm. is 100 to 120 C
        unsigned int pre_time; //time spend in pre-heat phase in seconds.
        //used to cure epoxi adhesive if necessary.
        // from 0 to 10 min.
        unsigned int soak_time; // time spend in soakphase in seconds.
        //Reccomm. is 1min to 4 min
        unsigned int soak_endtemp; // the endtemp in the soakphase en 1 C.
        //Recomm. is 175 to 180 C
        unsigned int reflow_time; // the time spend in reflowphase in seconds
        unsigned int reflow_peaktemp; // the max temperature reached in reflowphase.
    } profile;

    struct parametros {
        unsigned int indice [sizeof (struct SizeProfile)
        / sizeof (unsigned int) ];
    } param;
};

#define PROFILES_START_ADR 2// Start adres of profile-settings in EEPROM Memory
#define LOG_START_ADR 18// Start adres of temperaturelog in EEPROM Memory
#define LOG_END_ADR 65532	// End adres of temperaturelog in EEPROM Memory

/******************************************************************************/
/* User Function Prototypes                                                   */
/******************************************************************************/

/* I/O and Peripheral Initialization */
void InitApp(void);
void init_controller(void);
void controle(void);
void wait_1ms (void);
void wait_10ms (void);
void secbeat(void);
void calibrate(void);
void edit (void);
void send_log(void);
void logging(void);
void Delay_ms(long atraso);

extern char str_buf[20];
extern volatile unsigned int timer0count;
extern volatile unsigned int timercount80;
extern volatile bit timer0flag;
extern unsigned char power_heat1,power_heat2;
extern unsigned char oven_overshoot;
extern union Uperfis perfil;
//extern struct Tprofile profile;
extern bit tmp_log;
extern volatile bit secflag;
extern unsigned char* p_prof;
extern volatile bit Buzzer_stat;

typedef unsigned char bool;

#define true 1
#define false 0