/*
 * Ins.c
 *
 *  Created on: 2026��5��5��
 *      Author: misaka19754
 */

#include "zf_common_headfile.h"

float Motor_Standard_Speed = 33.0f;         //Ŀ�공��
float step = 8.0;//2





//����
#define GD_PI       3.1415926f
#define GD_DEG2RAD  (GD_PI / 180.0f)

guandao_mode gd_mode = guandao_pass_mode;

union FloatInspector data_x,data_y,data_yaw;  //��¼��ǰ��

float dis_record = 0.03;//��Ŀ1�Ϳ�Ŀ3��Ҫ�ĳ�0.2-20CM

int guandao_index = 0;         //��¼���Ŀ¼
int guandao_cnt = 0;           //��ǰѭ����
int guandao_last_cnt = 0;
int guandao_new_cnt = 0;
int forward = 3;               //ǰհ���ȣ�ԭ2��3����ǰת�������׼��
int window = 10;               //�ѵ㴰��

float guandao_lucheng = 0;       //��¼���˶��پ���
float X_Record_f[MAX] = {0};      //��ȡ�õ���x������
float Y_Record_f[MAX] = {0};      //��ȡ�õ���y������
float Yaw_Record_f[MAX] = {0};    //��ȡ�õ���yaw����
uint32 X_Record_i[MAX] = {0};       //Ҫ�ϴ���x������
uint32 Y_Record_i[MAX] = {0};       //Ҫ�ϴ���y������
uint32 Yaw_Record_i[MAX] = {0};     //Ҫ�ϴ���yaw����

float theta0 = 0;    //theta0   ��¼��ǰƫ����    ���Ŀ��ֵƫ����
float theta1 = 0;    //theta1   flash�е�ƫ����
float theta2 = 0;    //theta2   �������ó���ƫ����
float c_error = 0;
float int_c_error = 0, int_c_max = 40, int_c_min = -40;
float stl_kp = 15;//8 //8.5
float stl_ki = 2000;    //2000
/*˼·ʵ�֣�
 * 1.��¼ÿһ�����yaw
 * 2.��¼ÿһ�����x��y����
 * 3.��ȡ��һ�����x��y�����뵱ǰx��y�������ߣ��ó�ת��Ŀ��ֵ
 */

//���ߺ���
float gougu(float x1,float y1,float x2,float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

/*
 * x,y�ǵ�ǰ����
 * X_Record_f,Y_Record_f��·����������
 * len�����鳤��
 * per_cnt����һ��ƥ�������
 */
float cross_error(float x , float y , float *X_Record_f , float *Y_Record_f , int len , int pre_cnt , int *new_cnt)    //����������
{
    int start = pre_cnt - window;
    int end = pre_cnt + window;

    if (start < 0) start = 0;               //�޶�������Χ
    if (end >= len) end = len - 1;

    int best_cnt = pre_cnt;
    float min_dist2 = INFINITY;

    for (int i = start; i <= end; ++i)
    {
        float dx = x - X_Record_f[i];
        float dy = y - Y_Record_f[i];
        float d2 = dx*dx + dy*dy;
        if (d2 < min_dist2)
        {
            min_dist2 = d2;
            best_cnt = i;
        }
    }

    *new_cnt = best_cnt + forward;

    // ��ƥ�������߷������������
    float theta = Yaw_Record_f[best_cnt] * 3.1415926f / 180.0f;
    float dx = x - X_Record_f[best_cnt];
    float dy = y - Y_Record_f[best_cnt];
    float cross_track = dy * cosf(theta) - dx * sinf(theta);

    return cross_track;
}

//ʵ�ֺ���

//��¼����
void guandao_record(void)
{
    if(guandao_lucheng <= dis_record)
    {
        return;
    }

    guandao_lucheng -= dis_record;

    if(gd_mode == guandao_record_mode)
    {
        X_Record_i[guandao_index] = data_x.i;
        Y_Record_i[guandao_index] = data_y.i;
        Yaw_Record_i[guandao_index] = data_yaw.i;

        guandao_index ++;
    }
}

//�ϴ�����
void guandao_flash_record(void)
{
    uint16 lenh,lenx,leny,lenyaw;
    uint32 *ph,*px,*py,*pyaw;

    lenh = 1;
    lenx = MAX;
    leny = MAX;
    lenyaw = MAX;
    ph = (uint32)&guandao_index;
    px = X_Record_i;
    py = Y_Record_i;
    pyaw = Yaw_Record_i;

    flash_erase_page(0, FLASH_H_PAGE);
    flash_erase_page(0, FLASH_X_PAGE);
    flash_erase_page(0, FLASH_Y_PAGE);
    flash_erase_page(0, FLASH_YAW_PAGE);

    flash_write_page(0, FLASH_H_PAGE, ph, lenh);
    flash_write_page(0, FLASH_X_PAGE, px, lenx);
    flash_write_page(0, FLASH_Y_PAGE, py, leny);
    flash_write_page(0, FLASH_YAW_PAGE, pyaw, lenyaw);
}

//��ȡ����,��ת��Ϊfloat��ѭ��
void guandao_flash_load(void)
{
    uint16 lenh,lenx,leny,lenyaw;
    uint32 *ph,*px,*py,*pyaw;
    int i;

    lenh = 1;
    lenx = MAX;
    leny = MAX;
    lenyaw = MAX;
    ph = (uint32)&guandao_index;
    px = X_Record_i;
    py = Y_Record_i;
    pyaw = Yaw_Record_i;

    flash_read_page(0, FLASH_H_PAGE, ph, lenh);
    flash_read_page(0, FLASH_X_PAGE, px, lenx);
    flash_read_page(0, FLASH_Y_PAGE, py, leny);
    flash_read_page(0, FLASH_YAW_PAGE, pyaw, lenyaw);

    for(i = 0; i < guandao_index; i++)
    {
        data_x.i = X_Record_i[i];
        X_Record_f[i] = data_x.f;
        data_y.i = Y_Record_i[i];
        Y_Record_f[i] = data_y.f;
        data_yaw.i = Yaw_Record_i[i];
        Yaw_Record_f[i] = data_yaw.f;


 //��ӡÿ������������
        printf("INDEX %d: X=%f, Y=%f, YAW=%f\r\n", i, X_Record_f[i], Y_Record_f[i], Yaw_Record_f[i]);

        printf("(%f,%f)\r\n",X_Record_f[i], Y_Record_f[i]);


    }

}


float T_A=0;
void guandao_load(void)
{
    if(guandao_lucheng <= dis_record)
    {
        return;
    }

    guandao_lucheng -= dis_record;

    c_error = cross_error(data_x.f , data_y.f , X_Record_f , Y_Record_f , guandao_index , guandao_last_cnt , &guandao_new_cnt);//����������ͬʱ���µ�ǰ�ܵĵ�

    int_c_error += stl_ki * c_error * 0.001;//0.001���ж�û��ϵ�Ǹ�����

    if(int_c_error >= int_c_max){int_c_error = int_c_max;}
    if(int_c_error <= int_c_min){int_c_error = int_c_min;}

    /* ȡǰհ��� yaw ��Ϊ Stanley ����ο����� guandao_new_cnt = best_cnt + 2�� */
    int yaw_ref_idx = guandao_new_cnt;
    if(yaw_ref_idx >= guandao_index) { yaw_ref_idx = guandao_index - 1; }

    float temp = Yaw_Record_f[yaw_ref_idx] - atanf(stl_kp * c_error / Motor_Standard_Speed)*180.0f / 3.1415926f - int_c_error;//temp-Ŀ��Ƕ�
//        float temp = Yaw_Record_f[yaw_ref_idx] - atanf(stl_kp * c_error / 33)*180.0f / 3.1415926f - int_c_error;//temp-Ŀ��Ƕ�

    temp = AngleErrorNormalize(temp);


//    printf("temp=%f\r\n",temp);

    float diff = AngleErrorNormalize(temp - Taget_angle);


    if(fabsf(diff) <= step)
       {
        Taget_angle = temp;
       }
       else if(diff > 0.0f)
       {
           Taget_angle = AngleErrorNormalize(Taget_angle+ step);
       }
       else
       {
           Taget_angle = AngleErrorNormalize(Taget_angle - step);
       }
//    printf("T_A=%f\r\n",T_A);
    guandao_last_cnt = guandao_new_cnt;
}

//if(guandao_new_cnt>=-INDEX)
//{
//  �������
//}

//��ʼ������
//ȫ����ʼ��
void guandao_Init(void)
{
    memset(&X_Record_f, 0, sizeof(X_Record_f));
    memset(&Y_Record_f, 0, sizeof(Y_Record_f));

    memset(&Yaw_Record_f, 0, sizeof(Yaw_Record_f));
    memset(&X_Record_i, 0, sizeof(X_Record_i));

    memset(&Y_Record_i, 0, sizeof(Y_Record_i));
    memset(&Yaw_Record_i, 0, sizeof(Yaw_Record_i));

    guandao_lucheng = 0;
    guandao_index = 0;
    guandao_cnt = 0;
    guandao_last_cnt = 0;
    guandao_new_cnt = 0;

    data_x.f = 0;
    data_y.f = 0;

    gd_mode = guandao_pass_mode;    //Ĭ��Ϊ����״̬
}

void guandao_task(void)
{
    if(gd_mode == guandao_record_mode)
    {
        guandao_record();
    }
    else if(gd_mode == guandao_load_mode)
    {
        guandao_load();
    }
}

void Ins_text(void)
{
        if(key1_flag==1)//��ʼ¼��
        {
            key1_flag=0;

            Buzzer_check(50);
            gpio_set_level(LED1,0);

            /* ��ǰ���� -> ��ʼ¼�� */
            if(gd_mode == guandao_pass_mode)
            {
                /* ��ʼ¼�ƺ������� */
                gd_mode = guandao_record_mode;

                Mode_chage=1;//������ͷ�涯
            }

        }

        if(key2_flag==1)//���յ�ֹͣ¼�Ʋ�����
        {
            key2_flag=0;
            Buzzer_check(100);
            gpio_set_level(LED2,0);

            if(gd_mode == guandao_record_mode)
            {
                /* ����¼�ƽ���� Flash */
                   guandao_flash_record();

                   gd_mode = guandao_pass_mode;
            }

        }

        if(key3_flag==1)//����·��
        {
            key3_flag=0;
            Buzzer_check(300);
            gpio_set_level(LED3,0);


            if(gd_mode == guandao_pass_mode)
            {
                /* �� Flash �ж�ȡ¼�õĺ������У�����ʼ�ط� */
                guandao_flash_load();

                /* ���ùߵ�λ�úͻ��������¼�ơ��ط�֮����ۻ�Ư�� */
                data_x.f = 0;
                data_y.f = 0;
                guandao_lucheng = 0;
                c_error = 0;
                int_c_error = 0;
                guandao_last_cnt = 0;
                guandao_new_cnt = 0;

                gd_mode = guandao_load_mode;


                Mode_chage=2;



            }




        }




}


void INS_log(void)
{
    ips_show_string(8*0, 16*0, "Distance:");        ips_show_float(8*10,16*0, guandao_lucheng,3,6);
    ips_show_string(8*0, 16*1, "YAW:");             ips_show_float(8*10,16*1, theta0,3,6);
    ips_show_string(8*0, 16*2, "X:");               ips_show_float(8*10,16*2, data_x.f ,3,6);
    ips_show_string(8*0, 16*3, "Y");                ips_show_float(8*10,16*3, data_y.f ,3,6);
    ips_show_string(8*0, 16*4, "D_YAW");            ips_show_float(8*10,16*4, data_yaw.f,3,6);
    ips_show_string(8*0, 16*5, "INDEX");            ips_show_uint(8*10,16*5, guandao_index,5);
    ips_show_string(8*0, 16*6, "T_A");              ips_show_float(8*10,16*6, Taget_angle,3,6);

}


void INS_data_get(void)
{
    // float d = Cal_Distance(g_encoder_raw);              // ��������ࣨ�ɣ�
    float d = CYT2_get_distance_mag(motor_value.receive_left_speed_data); // �ű��ࣨ�£�

    guandao_lucheng += d;                               //���־���

    theta0 = roll_balance_cascade.posture_value.yaw;    //��ȡƫ����
    data_x.f += d * cosf(theta0 * 3.1415926f / 180.0f); //��λ�����X���λ��
    data_y.f += d * sinf(theta0 * 3.1415926f / 180.0f); //��λ�����Y���λ��
    data_yaw.f = theta0;
}
