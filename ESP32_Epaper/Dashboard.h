#ifndef Dashboard_h
#define Dashboard_h

#include "CANopen.h"


#define TEMPR 2
#define TEMPG 17
#define CURRR 25
#define CURRG 26
#define TEMPTHRESHY 67
#define TEMPTHRESHR 68
#define CURRTHRESHY 25
#define CURRTHRESHR 250
#define BUTTON1 34
#define BUTTON2 35

void buttonOne();
void buttonTwo();

 

class Dashboard
{

  public:
    // Define Constructor
    Dashboard();
    // Define Public Functions
    void startDashboard();


  private:
    void runDisplay();
    void showPartialUpdate();
    void handleWarning();
    void setupDashboard();

};

#endif
