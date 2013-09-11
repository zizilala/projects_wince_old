;
;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
;
;   File:  interrupts.s
;
        INCLUDE kxarm.h
       
        TEXTAREA
        
;-------------------------------------------------------------------------------
;
;  Function:  INTERRUPTS_STATUS
;
;  returns current arm interrupts status.
;
 LEAF_ENTRY INTERRUPTS_STATUS

        mrs     r0, cpsr                    ; (r0) = current status
        ands    r0, r0, #0x80               ; was interrupt enabled?
        moveq   r0, #1                      ; yes, return 1
        movne   r0, #0                      ; no, return 0
        mov     pc, lr
        
 ENTRY_END INTERRUPTS_STATUS
 
 
        END


