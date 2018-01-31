
uint timer_ms;              //pocet ms od zapnuti
uint day_ms;                //pocet ms v tomto dni, od 0:00:00 hod
void* minute_event=NULL;    //adresa fce, ktera se vola kazdou minutu (RTC alarm)

void timer_Init();