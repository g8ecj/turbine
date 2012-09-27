

// all the fields displayed
enum VARS
{
	eVOLTS = 1,
	eCHARGE,
	ePOWER,

	eAMPS,
	eSHUNT,
	eRPM,

	eMAXHOUR,
	eMAXDAY,
	eMINHOUR,
	eMINDAY,
	eTOTAL,
	eUSED,

	eTEMPERATURE,

	eVOLT_LIMIT_LO,
	eVOLT_LIMIT_HI,
	eVOLT_FLOAT,
	eVOLT_ABSORB,
	eMIN_CHARGE,
	eBANK_SIZE,

	eSYSTEM_VOLTS,
	eCAL_VOLTS,

	eHOUR,
	eMINUTE,
	eSECOND,
	eDAY,
	eMONTH,
	eYEAR,
	eSYNC,
	eMANUAL,
	eINVERTER,
	eNUMVARS
};



void ui_init(void);
void run_ui(void);
void set_flash(int8_t field, int8_t set);
bool check_value(enum VARS var, int16_t value);
void load_eeprom_values(void);


