

void pwm_on(char index, char val);
void pwm_setPower(char index, int val);
void pwm_setPowerLinearProc(char index, int target_proc, int step_ms, int step_size);
void pwm_setPowerLinear(char index, int target_val, int step_ms, int step_size);
void pwm_setPowerExp(char index, int target_index, int step_ms);
int pwm_getProc(char index);
int pwm_getValue(char index);
void pwm_Init();