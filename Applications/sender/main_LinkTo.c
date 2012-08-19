/*----------------------------------------------------------------------------
 *  Demo Application for SimpliciTI 
 * 
 *  L. Friedman 
 *  Texas Instruments, Inc.
 *---------------------------------------------------------------------------- */

/********************************************************************************************
  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED �AS IS�
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"

//#include "app_remap_led.h"

static void linkTo(void);

void toggleLED(uint8_t);

static uint8_t  sTxTid, sRxTid;
static linkID_t sLinkID1;

/* application Rx frame handler. */
static uint8_t sRxCallback(linkID_t);

#define SPIN_ABOUT_A_SECOND  NWK_DELAY(100)

void main (void)
{
  WDTCTL = WDTPW + WDTHOLD;
  BSP_Init();

  /* If an on-the-fly device address is generated it must be done before the
   * call to SMPL_Init(). If the address is set here the ROM value will not 
   * be used. If SMPL_Init() runs before this IOCTL is used the IOCTL call 
   * will not take effect. One shot only. The IOCTL call below is conformal. 
   */
#ifdef I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE
  {
    addr_t lAddr;

    createRandomAddress(&lAddr);
    SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
  }
#endif /* I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE */

  /* This call will fail because the join will fail since there is no Access Point 
   * in this scenario. But we don't care -- just use the default link token later. 
   * We supply a callback pointer to handle the message returned by the peer. 
   */

  SMPL_Init(sRxCallback);


  /* turn on LEDs alternatively */
  toggleLED(1);
  toggleLED(2);
  NWK_DELAY(200);
  int i,j;
  for (i = 6; --i >= 0; ) {
    for (j = 1; j<=3 ; j++) {
      toggleLED(j);
      NWK_DELAY(120);
    }
  }


  /* never coming back... */
  linkTo();

  /* but in case we do... */
  while (1) ;
}

static void linkTo()
{
  uint8_t  msg[2];

  while (SMPL_SUCCESS != SMPL_Link(&sLinkID1))
  {
    /* blink red LED, until we link successfully */
    toggleLED(1);
    NWK_DELAY(1000);//SPIN_ABOUT_A_SECOND;
  }

  /* we're linked. turn off LEDs. Received messages will toggle the green LED. */
  if (BSP_LED1_IS_ON())
  {
    toggleLED(1);
  }

  if (BSP_LED2_IS_ON())
  {
    toggleLED(2);
  }
  
  /* turn on RX. default is RX off. */
  SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);

  /* put LED to toggle in the message */
  msg[0] = 2;  /* toggle green */
  while (1)
  {
    SPIN_ABOUT_A_SECOND;

    /* put the sequence ID in the message */
    msg[1] = ++sTxTid;
    SMPL_Send(sLinkID1, msg, sizeof(msg));
  }
}


void toggleLED(uint8_t which)
{
  switch(which)
   {
    case 1 :
      BSP_TOGGLE_LED1();
      break;
    case 2 :
      BSP_TOGGLE_LED2();
      break;
   }
  return;
}


/* handle received frames. */
static uint8_t sRxCallback(linkID_t port)
{
  uint8_t msg[2], len, tid;

  /* is the callback for the link ID we want to handle? */
  if (port == sLinkID1)
  {
    /* yes. go get the frame. we know this call will succeed. */
     if ((SMPL_SUCCESS == SMPL_Receive(sLinkID1, msg, &len)) && len)
     {
       /* Check the application sequence number to detect
        * late or missing frames... 
        */
       tid = *(msg+1);
       if (tid)
       {
         if (tid > sRxTid)
         {
           /* we're good. toggle LED in the message */
           toggleLED(*msg);
           sRxTid = tid;
         }
       }
       else
       {
         /* the wrap case... */
         if (sRxTid)
         {
           /* we're good. toggle LED in the message */
           toggleLED(*msg);
           sRxTid = tid;
         }
       }
       /* drop frame. we're done with it. */
       return 1;
     }
  }
  /* keep frame for later handling. */
  return 0;
}

