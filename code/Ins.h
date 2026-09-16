
/*
 * Ins.h
 *
 *  Created on: 2026��5��5��
 *      Author: misaka19754
 */

#ifndef CODE_INS_H_
#define CODE_INS_H_

#define dis_position 0.01       //����λ�þ���
#define MAX 1000         //��������
#define FLASH_H_PAGE 8   //��ͷ������
#define FLASH_X_PAGE 9   //��x��ҳ��
#define FLASH_Y_PAGE 10  //��y��ҳ��
#define FLASH_YAW_PAGE 11  //��y��ҳ��

typedef enum
{
    guandao_pass_mode = 0,     // ����
    guandao_record_mode,       // ¼�ƺ���
    guandao_load_mode          // �ط�ѭ��
} guandao_mode;

union FloatInspector {
    float    f;   // 4 �ֽ�
    uint32   i;   // 4 �ֽ�
};

extern guandao_mode gd_mode;
extern union FloatInspector data_x,data_y,data_yaw;

extern float dis_record;
extern int guandao_index;         //��¼���Ŀ¼
extern int guandao_cnt;           //��ǰѭ����
extern int guandao_last_cnt;
extern int guandao_new_cnt;
extern float guandao_lucheng;
extern float X_Record_f[MAX];
extern float Y_Record_f[MAX];
extern float Yaw_Record_f[MAX];
extern uint32 Yaw_Record_i[MAX];

extern float Motor_Standard_Speed;

extern float theta0;    //theta0   ��¼��ǰƫ����    ���Ŀ��ֵƫ����
extern float theta1;    //theta1   flash�е�ƫ����
extern float theta2;    //theta2   �������ó���ƫ����
extern float c_error;
extern float int_c_error;
extern float stl_kp;    //8.5
extern float stl_ki;
extern float step;

void guandao_record(void);
void guandao_flash_record(void);
void guandao_flash_load(void);
void guandao_load(void);
void guandao_Init(void);
void guandao_task(void);

void Ins_text(void);
void INS_log(void);
void INS_data_get(void);

#endif /* CODE_INS_H_ */
