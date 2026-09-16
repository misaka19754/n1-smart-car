/*
 * Subject_3.c
 *
 *  Created on: 2026��6��24��
 *      Author: misaka19754
 *
 *  ��Ŀ3����Ŀ1��3���������� + �ߵ��Ƕ� + ����Զ�����
 *    ���ƣ�balance_mode_parameter(3) �� �н��ٶȻ�
 *    �Ƕȣ����� INS��guandao_load ���� Taget_angle��
 *    �ٶȣ�ֱ�� SPEED_MAX������Զ����� SPEED_MIN
 */

#include "zf_common_headfile.h"

/* ����Զ����ٲ��� */
#define SPEED_MAX   50
#define SPEED_MIN   45


void Body_ctrl_3(void)
{
    static bool once = false;
    if (!once)
    {
      balance_mode_parameter(3);            // ��Ŀ3�������н��ٶȻ���
      Taget_angle = roll_balance_cascade.posture_value.yaw;
      once = true;
    }


    Imu_lowpass_filter();                   // IMU����һ�׵�ͨ�˲�


    /* ===== ����Զ����� ===== */

//    float turn_err = fabsf(AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw));
//    float speed_target = SPEED_MAX;
//    if(turn_err > 20.0f)
//    {
//        speed_target = SPEED_MIN;
//    }
//    else if(turn_err > 0.5f)
//    {
//        speed_target = SPEED_MAX - (SPEED_MAX - SPEED_MIN) * (turn_err - 0.5f) / 19.5f;
//    }


    /* ===== 3���������ƣ�ͬ��Ŀ1�� ===== */

    if(sys_times%1==0)//���ٶȻ�
    {
        if(Imu_type==1)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660ra_gyro_x);
        }
        else if(Imu_type==2)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu660rb_gyro_y);
        }
        else if(Imu_type==3)
        {
            pid_control(&roll_balance_cascade.angular_speed_cycle, roll_balance_cascade.angle_cycle.out, imu963ra_gyro_y);
        }
    }

    if(sys_times%5==0)//�ǶȻ�
    {
        pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);
    }

    if(sys_times%20==0)//ת�� �� Taget_angle �� INS ���£����� StepApproach
    {
        pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw), 0);
    }


     Steer_set(SERVO_MOTOR_MID + roll_balance_cascade.angular_speed_cycle.out);

     /* ·���յ��⣬�Զ�ͣ�� */
     if(guandao_new_cnt >= guandao_index)
     {
         CYT2_S_motor_loop_ctrl(0);
     }
     else
     {
//         CYT2_S_motor_loop_ctrl(Motor_Standard_Speed);//�ջ��ٶ�
         CYT2_S_motor_ctrl(1200);//�����ٶ�
     }


}
