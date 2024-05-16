#include "iarduino_VpH.h"																									//

#define  overMillis(a,b) ((uint32_t)(millis()-(a))>(uint32_t)(b))
																															//
//		ФУНКЦИЯ ЗАПУСКА КАЛИБРОВКИ:																							//
void	iarduino_VpH::setCalibration(void){																					//
			if( mode_Cal == 0 ){ mode_Cal = 1;                    }															//	Если калибровка не выполнялась, то начинаем 1 стадию калибровки.
			if( mode_Cal == 3 ){ mode_Cal = 4; time_Cal=millis(); }															//	Если ожидается смена жидкости, то ждём 10 секунд и начинаем 2 стадию калибровки.
}																															//
																															//
//		ФУНКЦИЯ ПОЛУЧЕНИЯ ТЕКУЩЕЙ СТАДИИ КАЛИБРОВКИ:																		//	Возвращает стадию калибровки: 1 или 2, 0-не выполняется, 3-ожидание смены жидкости.
uint8_t	iarduino_VpH::getCalibration(void){																					//
			if( mode_Cal == 0 ){ return 0; }																				//	Калибровка не выполняется.
			if( mode_Cal <  3 ){ return 1; }																				//	1,2: Выполняется 1 стадия калибровки.
			if( mode_Cal == 3 ){ return 3; }																				//	3  : Ожидается смена калибровочной жидкости.
			if( mode_Cal == 4 ){ return 4; }																				//	4  : Ждём 10 секунд перед началом 2 стадии калибровки.
			if( mode_Cal < 10 ){ return 2; }																				//	5-7: Выполняется 2 стадия калибровки.
}																															//
																															//
//		ФУНКЦИЯ УКАЗАНИЯ ДИФФЕРЕНЦИАЛЬНОГО НАПРЯЖЕНИЯ НА ВЫВОДАХ ЩУПА:														//
void	iarduino_VpH::setDif_V(float v){																					//	Параметр: v ± разница потенциала выхода щупа, относительно его входа в Вольтах.
			pred_pH = real_pH;																								//
		//	Вычисляем кислотность:																							//
			real_pH = pHn - v / (mVstp/1000.0f);																			//	pH = 7pH - ΔV/Vstp.
		//	Усредняем кислотность:																							//
			if( averaging ){aver_pH*=float(averaging);aver_pH+=real_pH;aver_pH/=float(averaging)+1.0;}else{aver_pH=real_pH;}//	Усредняем pH если коэффициент усреднения выше 0.
		//	Усредняем напряжение:																							//
			static float aver_V=v; aver_V*=float(averaging);aver_V+=v;aver_V/=float(averaging)+1.0;							//	Усредняем напряжение для режима ожидания смены калибровочной жидкости.
		//	Определяем коэффициент нормализации кислотности:																//	
			if( overMillis(time_Nor, 500) ){																				//	Если с последнего отбора значения прошло более 500 мс.
				time_Nor=millis();																							//	Время последнего отбора кислотности для определения её нормализации.
				drif_pH=0;for(uint8_t i=1;i<20;i++){drif_pH+=(aray_pH[i-1]-aray_pH[i]);aray_pH[i-1]=aray_pH[i];}drif_pH/=19;//	Вычисляем дрейф pH и сдвигаем данные по массиву вперёд.
				aray_pH[19]=real_pH;																						//	Добавляем последнее отобранное значение pH.
				if( fabs(drif_pH) < (min_drift     ) ){ flag_NorOk=1; }														//	Если дрейф pH <    min_drift pH, то считаем показания стабильными.
				if( fabs(drif_pH) > (min_drift*1.5f) ){ flag_NorOk=0; time_NorOk=millis(); }								//	Если дрейф pH > 1.5min_drift pH, то считаем показания не стабильными.
			}																												//
		//	Устанавливаем флаги ошибок:																						//	
			err = 0;																										//
			if( (aver_pH < 0          ) || (aver_pH > 14.0f      ) ){ err |= VpH_Err_Limit; }								//	Устанавливаем флаг VpH_Err_Limit если pH показания выходят за пределы 0...14.
			if( (pred_pH > (real_pH+3)) || (real_pH > (pred_pH+3)) ){ err |= VpH_Err_jumps; }								//	Устанавливаем флаг VpH_Err_jumps если показания pH "скачут".
		//	Выполняем калибровку:																							//
			if( mode_Cal ){																									//
			//	1	Начинаем первую стадию калибровки:																		//
				if( mode_Cal==1 ){																							//	Если калибровка была запущена...
					if( overMillis(time_NorOk,4000) ){																		//	Если с момента стабилизации показаний pH прошло более 4 секунд...
						mode_Cal=2;																							//	Переходим к накаплению разницы потенциалов на выходе щупа ΔV1.
						aray_Cal[0]=0; coun_Cal=0;																			//	Сбрасываем «aray_Cal[0]» для накапления ΔV1 и обнуляем счётчик «coun_Cal».
						time_Cal=millis();																					//	Определяем время «time_Cal».
					}																										//
				}																											//	
			//	2	Читаем разницу потенциалов на выходе щупа «ΔV1»:														//	
				if( mode_Cal==2 ){																							//	Если выполняется первая стадия калибровки.
					if( coun_Cal==0 ){ time_Cal=millis(); }																	//	Первое вхождение.
					if( time_Cal <= millis() ){																				//	Если наступило время «time_Cal», то ...
						time_Cal  = millis()+100;																			//	Определяем новое время «time_Cal».
						coun_Cal++; 																						//	Увеличиваем счётчик.
						aray_Cal[0]+=v;																						//	Накапливаем ΔV1.
						if( coun_Cal==100 ){ aray_Cal[0]/=coun_Cal; mode_Cal=3; }											//	Получаем среднее арифметическое значение «ΔV1» и переходим к ожиданию смены жидкости.
					}																										//	
				}																											//	
			//	3	Ждём опускание щупа во вторую калибровочную жидкость:													//	
				if( mode_Cal==3 ){																							//	Если первая часть калибровки завершена, то сообщаем о простое...
					if( aray_Cal[0]<aver_V ){ if( (aver_V-aray_Cal[0]) > (stand_mVstp/1000.0f) ){ mode_Cal=4; } }			//	Если накопленное ΔV1 < текушего aver_V более чем на эталонное mVstp (1pH), то начинаем 2 стадию.
					else                    { if( (aray_Cal[0]-aver_V) > (stand_mVstp/1000.0f) ){ mode_Cal=4; } }			//	Если накопленное ΔV1 > текушего aver_V более чем на эталонное mVstp (1pH), то начинаем 2 стадию.
					if( mode_Cal==4        ){ if( !overMillis(time_NorOk, 4000)                ){ mode_Cal=3; } }			//	Не даём перейти к 2 стадии калибровки, если показания не стабилизированы в течении 4 секунд.
					if( mode_Cal==4        ){ time_Cal=millis(); }															//	Если начинаем 2 стадию калибровки, то запоминаем текущее время.
				}																											//	
			//	4	Ждём 10 секунд:																							//	
				if( mode_Cal==4 ){																							//	Если зафиксировано изменение кислотности на 1 pH (смена калибровочной жидкости)...
					if( overMillis(time_Cal, 10000) ){ mode_Cal=5; }														//	Если с момента времени time_Cal прошло более 10 сек, переходим к выполнению 2 стадии калибровки.
				}																											//	
			//	5	Начинаем вторую стадию калибровки:																		//	
				if( mode_Cal==5 ){																							//	Если жидкость была изменена...
					if( overMillis(time_NorOk,4000) ){																		//	Если с момента стабилизации показаний pH прошло более 4 секунд...
						mode_Cal=6;																							//	Переходим к накаплению разницы потенциалов на выходе щупа ΔV2.
						aray_Cal[1]=0; coun_Cal=0;																			//	Сбрасываем «aray_Cal[1]» для накапления ΔV2 и обнуляем счётчик «coun_Cal».
						time_Cal=millis();																					//	Определяем время «time_Cal».
					}																										//
				}																											//	
			//	6	Читаем смещение напряжения датчика «ΔV2»:																//	
				if( mode_Cal==6 ){																							//	Если выполняется вторая стадия калибровки.
					if( coun_Cal==0 ){ time_Cal=millis(); }																	//	Первое вхождение.
					if( time_Cal <= millis() ){																				//	Если наступило время «time_Cal», то ...
						time_Cal  = millis()+100;																			//	Определяем новое время «time_Cal».
						coun_Cal++; 																						//	Увеличиваем счётчик.
						aray_Cal[1]+=v;																						//	Накапливаем ΔV2.
						if( coun_Cal==100 ){ aray_Cal[1]/=coun_Cal; mode_Cal=7; }											//	Получаем среднее арифметическое значение «ΔV2» и переходим к выполнению расчётов.
					}																										//	
				}																											//	
			//	7	Выполняем рассчёты:																						//	
				if( mode_Cal==7 ){																							//	Если все данные готовы...
					mode_Cal   = 0;																							//	
					flag_CalOk = true;																						//
					float cal_mVstp = ( (aray_Cal[0]-aray_Cal[1])/(known_PH2-known_PH1) )*1000.0f;							//	Vstp = (ΔV1-ΔV2)/(pH2-pH1).
					float cal_pHn   =   known_PH1 - (aray_Cal[0] *(known_PH2-known_PH1) )/(aray_Cal[1]-aray_Cal[0]);		//	pHn  = pH1 - ( ΔV1*(pH2-pH1) )/(ΔV2-ΔV1) , где: ΔVout = Vn-Vout, pH = REG_KNOWN_PH.
				//	Проверяем результат калибровки:																			//
					if( cal_mVstp < (stand_mVstp*0.4f) ){ flag_CalOk=false; }												//	Ошибка, слишком низкий  шаг смещения напряжения датчика на 1pH.
					if( cal_mVstp > (stand_mVstp*2.5f) ){ flag_CalOk=false; }												//	Ошибка, слишком высокий шаг смещения напряжения датчика на 1pH.
					if( cal_pHn   < (7.0f       *0.5f) ){ flag_CalOk=false; }												//	Ошибка, слишком низкая  нейтральная кислотность датчика.
					if( cal_pHn   > (7.0f       *1.5f) ){ flag_CalOk=false; }												//	Ошибка, слишком высокая нейтральная кислотность датчика.
				//	Сохраняем результат калибровки:																			//	
					if( flag_CalOk ){ mVstp=cal_mVstp; pHn=cal_pHn; }														//	
				}																											//	
			}																												//
}																															//