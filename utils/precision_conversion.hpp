#include <limits>
#include <systemc.h>
#include <iostream>

inline int64_t convertUint2Int(uint64_t data) {
	constexpr auto max_int = std::numeric_limits<int64_t>::max();
	assert(data < max_int);
	return static_cast<int64_t>(data);
}

inline float convertSc_bv_16toFp32(sc_bv<16> data)
{
	typedef union
	{
		uint16_t data16[2];
		float data32;

	}_MQ;
	_MQ mq;
	float temp;
	mq.data16[1] = data.to_uint();
	mq.data16[0] = 0;
	temp = mq.data32;
	return temp;
} 

inline sc_bv<16> convertFp32toSc_bv_16(float data)//���˽�λ��ϵ
{
	typedef union
	{
		uint32_t data32;
		float data_32;
	}_MQ;  
	_MQ mq;
	sc_bv<16> temp;
	sc_bv<32> temp1;
	mq.data_32 = data;
	temp1 = mq.data32;
	sc_bv<8>e = temp1.range(30, 23);//ָ��
	uint8_t e_uint = e.to_uint();//ָ��תΪint��������
	sc_bv<1>m6;//��Чβ��
	m6 = temp1.range(16,16);
	sc_bv<1>m7;//��Чβ��
	m7 = temp1.range(15, 15);
	sc_bv<22>m8_m22 = temp1.range(14, 0);//��m7֮�����Чβ��
	sc_bv<8>m_en;//��Чβ��
	m_en.range(7, 7) = "1";
	m_en.range(6, 0) = temp1.range(22, 16);
	uint8_t m_en_uint= m_en.to_uint();//����Чβ�����λ��1��ת����uint��������

	if (m7 == "0")
	{
		temp = temp1.range(31, 16);
		//cout << temp << endl;
	}
	else if (m7 == "1")
	{
		if (m8_m22 != "000000000000000")//���ǽ�λ����������,������ҹ�
		{
			if (m_en!="11111111")//δ�����ҹ�����
			{
				m_en_uint = m_en_uint + 1;//β����Чλ+1��λ
				sc_bv<8> bv1(m_en_uint);
				temp.range(6, 0) = bv1.range(6, 0);
				temp.range(15, 7) = temp1.range(31, 23);
			}
			else if (m_en == "11111111")//�����ҹ�����
			{
				e_uint = e_uint + 1;
				sc_bv<8> bv2(e_uint);
				temp.range(15, 15) = temp1.range(31,31);
				temp.range(14, 7) = bv2.range(7, 0);
				temp.range(6, 0) = "0000000";
			}			
		}
		else if (m8_m22 == "000000000000000")
		{
			if (m6=="0")
			{
				temp = temp1.range(31, 16);
			}
			else if (m6 == "1")//���ǽ�λ����������,������ҹ�
			{

				if (m_en != "11111111")//δ�����ҹ�����
				{
					m_en_uint = m_en_uint + 1;//β����Чλ+1��λ
					sc_bv<8> bv1(m_en_uint);
					temp.range(6, 0) = bv1.range(6, 0);
					temp.range(15, 7) = temp1.range(31, 23);
				}
				else if (m_en == "11111111")//�����ҹ�����
				{
					e_uint = e_uint + 1;
					sc_bv<8> bv2(e_uint);
					temp.range(15, 15) = temp1.range(31, 31);
					temp.range(14, 7) = bv2.range(7, 0);
					temp.range(6, 0) = "0000000";
				}
				
			}
		}
	}
 	return temp;
}
/*inline sc_bv<16> convertFp32toSc_bv_16(float data)//δ�ӽ�λ��ϵ
{
	typedef union
	{
		uint16_t data16[2];
		float data32;
	}_MQ;
	_MQ mq;
	sc_bv<16> temp;
	mq.data32 = data;
	temp = mq.data16[1];
	return temp;
}*/
