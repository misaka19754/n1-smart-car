/*
 * Subject_1.c
 *
 *  Created on: 2026��4��28��
 *      Author: misaka19754
 */

#include "zf_common_headfile.h"

/* ����Զ����ٲ��� */
//#define SPEED_MAX   80
//#define SPEED_MIN   45

#define SPEED_MAX   798
#define SPEED_MIN   439

uint32 Mode_chage=0;

uint32 sys_times=0;
float  End_error=0;
float  Taget_angle=0;



void Body_ctrl_1(void)
{
    static int32 I=0;   //ϵͳ��ʱ (ms)
    I++;


    static bool once = false;     //��ֻ֤��ֵһ��
    if (!once)
    {
      Taget_angle=0;              //��ʼ�Ƕ�Ϊ0
      balance_mode_parameter(1);   //�����Ŀһ����
      once = true;
    }


    Imu_lowpass_filter();       //IMU����һ�׵�ͨ�˲�


    /* ===== ����Զ����� ===== */

    float turn_err = fabsf(AngleErrorNormalize(Taget_angle - roll_balance_cascade.posture_value.yaw));
    float speed_target = SPEED_MAX;
    if(turn_err > 20.0f)
    {
        speed_target = SPEED_MIN;                           // ���>20�� �� ���������
    }
    else if(turn_err > 0.5f)
    {
        speed_target = SPEED_MAX - (SPEED_MAX - SPEED_MIN) * (turn_err - 0.5f) / 19.5f;
    }
    // ���<0.5�� �� ֱ����ȫ��
//    CYT2_S_motor_loop_ctrl(speed_target);      //�ջ���Ŀһ�ٶ�
    CYT2_S_motor_ctrl(1300);

    if(I>=5000)                 //�ı�I����ֵ���ɸı��Ŀһ��ͷʱ��
    {
        Taget_angle=180;
        gpio_set_level(Buzzer_pin,1);
    }


    if(sys_times%1==0)//���ٶȻ�
    {

        if(Imu_type==1)//��ѡ��IMU�ͺŵ�ʱ����Ѿ�ȷ��
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

    if(sys_times%20==0)//ת��
    {
//        pid_control(&roll_balance_cascade.turn_cycle, StepApproach(Taget_angle,2), roll_balance_cascade.posture_value.yaw);

//            if(sys_times>150)
//            {
//                pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(StepApproach(Taget_angle,4)-roll_balance_cascade.posture_value.yaw),0);
//            }
//            else
            {
                pid_control(&roll_balance_cascade.turn_cycle, AngleErrorNormalize(StepApproach(Taget_angle,2)-roll_balance_cascade.posture_value.yaw),0);
            }


    }


         Steer_set(SERVO_MOTOR_MID +roll_balance_cascade.angular_speed_cycle.out);


}


void Body_keep(void)//ֻ�������ٶȺͽǶ�ά�ֳ�ͷ�涯
{
    static int32 I=0;   //ϵͳ��ʱ
    I++;


    static bool once = false;     //��ֻ֤��ֵһ��
    if (!once)
    {
      balance_mode_parameter(1);   //�����Ŀһ����
      once = true;
    }


    Imu_lowpass_filter();       //IMU����һ�׵�ͨ�˲�



    if(sys_times%1==0)//���ٶȻ�
    {

        if(Imu_type==1)//��ѡ��IMU�ͺŵ�ʱ����Ѿ�ȷ��
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
            pid_control(&roll_balance_cascade.angle_cycle, 0-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);

//          pid_control(&roll_balance_cascade.angle_cycle, roll_balance_cascade.turn_cycle.out-roll_balance_cascade.posture_value.mechanical_zero, -roll_balance_cascade.posture_value.rol);

    }



         Steer_set(SERVO_MOTOR_MID +roll_balance_cascade.angular_speed_cycle.out);

}


void Balance_1_text(void)
{
    ips200_draw_line(0, 16*11, 239, 16*11, RGB565_RED);
    ips_show_string(8*0, 16*12, "YAW:");      ips_show_float(8*10,16*12, roll_balance_cascade.posture_value.yaw,3,6);
    ips_show_string(8*0, 16*13, "ROLL:");     ips_show_float(8*10,16*13, roll_balance_cascade.posture_value.rol,3,6);
    ips_show_string(8*0, 16*14, "A_OUT:");    ips_show_float(8*10,16*14, roll_balance_cascade.angle_cycle.out,3,6);
    ips_show_string(8*0, 16*15, "G_out");     ips_show_float(8*10,16*15, roll_balance_cascade.angular_speed_cycle.out,3,6);
    ips_show_string(8*0, 16*16, "Taget_A");   ips_show_float(8*10,16*16, Taget_angle,3,6);
    ips_show_string(8*0, 16*17, "DIS");       ips_show_float(8*10,16*17, guandao_lucheng,3,6);

    ips200_draw_line(0, 16*18, 239, 16*18, RGB565_RED);

}
