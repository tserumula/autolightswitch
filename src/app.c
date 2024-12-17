/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include <xc.h>
#include<math.h>


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

#define GREEN_LED PORTBbits.RB7
#define RED_LED PORTBbits.RB8
#define SWITCH PORTBbits.RB13

#define RELAY_01 PORTBbits.RB9

// Device = PIC32MX250F128B

int count;
int debounce_count;
short int status;
int adc_samples[3] = {0,0,0};
float adc_volts[3] = {0,0,0};

float threshold;
float step_size;
bool appInitialized = true;
float VDD_Voltage = 3.28;
int ADC_bits = 10;

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/

int getADC( int channel){
    
    DRV_ADC_INPUTS_POSITIVE input;
    int output, temp = 0;
    
    switch( channel){
        case 0 : input = ADC_INPUT_POSITIVE_AN0; break;
        case 1 : input = ADC_INPUT_POSITIVE_AN1; break;
        case 2 : input = ADC_INPUT_POSITIVE_AN2; break;
        case 3 : input = ADC_INPUT_POSITIVE_AN3; break;
        case 4 : input = ADC_INPUT_POSITIVE_AN4; break;
        default : return -1;
    }
           
    DRV_ADC_Open();
    DRV_ADC_PositiveInputSelect( ADC_MUX_A, input );
                
    DRV_ADC_Start();
    while( !DRV_ADC_SamplesAvailable() ){
        temp = 1;   
    }
                                                              
    DRV_ADC_Stop();
                
    output = DRV_ADC_SamplesRead(0);
    return output;
}



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            step_size = VDD_Voltage/ pow(2,ADC_bits);       
        
            count = 0;
            debounce_count = 0;
            status = 0;
            threshold = 3.15; // threshold voltage for daylight.
            
            RELAY_01 = 1; // turn off
            
            if (appInitialized){
                appData.state = APP_STATE_SERVICE_TASKS;               
            }
            
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            count ++;
            
            if( count > 5e3 ){   // Take samples in this block         
                adc_samples[0] = getADC(0);
                adc_volts[0] = adc_samples[0] * step_size;                
                count = 0;
                
                if( adc_volts[0] > threshold && debounce_count < 10 ){
                    //avoid counting up till memory overflow.
                    debounce_count++;
                }
                
                if( adc_volts[0] < threshold && debounce_count > 0 ){ //previously on
                    debounce_count--;
                }                
            }            
            
            // If count is below 300, lights off. Else lights on.
            // waits for approx 15s x 4 x (5) = 5 minutes.            
            if( debounce_count > 4 ){
                status = 1;
            }else{ 
                status = 0;
            }
            
            
            if( status == 1 ){ // night time                               
                
                status = 1;
                GREEN_LED = 1;
                RED_LED = 0;
                RELAY_01 = 0; //turn on relay switch                 
            }else{ //daylight 
                
                GREEN_LED = 0;
                RED_LED = 1;
                RELAY_01 = 1; // turn off relay switch
                status = 0;                                           
            }
            
            if( SWITCH == 0 ){
                appData.state = APP_STATE_ON_OVERIDE;
            }
            
            break;
        }

        /* TODO: implement your application state machine.*/
        
        case APP_STATE_ON_OVERIDE:
        {
           GREEN_LED = 1;
           RED_LED = 1;
           RELAY_01 = 0; //turn on relay switch            
        
           if( SWITCH == 1 ){
               appData.state = APP_STATE_SERVICE_TASKS;
           }                      
           
           break;
        }

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

 

/*******************************************************************************
 End of File
 */
