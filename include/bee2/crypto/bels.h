/*
*******************************************************************************
\file bels.h
\brief STB 34.101.60 (bels): secret sharing algorithms
\project bee2 [cryptographic library]
\created 2013.05.14
\version 2023.02.02
\copyright The Bee2 authors
\license Licensed under the Apache License, Version 2.0 (see LICENSE.txt).
*******************************************************************************
*/

/*!
*******************************************************************************
\file bels.h
\brief Алгоритмы СТБ 34.101.60 (bels)
*******************************************************************************
*/

#ifndef __BEE2_BELS_H
#define __BEE2_BELS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bee2/defs.h"

/*!
*******************************************************************************
\file bels.h

Реализованы алгоритмы СТБ 34.101.60 (bels). При ссылках на алгоритмы
подразумеваются разделы СТБ 34.101.60-2013, в которых эти алгоритмы определены.

Алгоритмы предназначены для разделения секрета из len октетов, 
где len = 16, 24 или 32. При разделении секрета используются открытые ключи mi,
которые также состоят из len октетов. Открытый ключ m0 является общим, 
а остальные открытые ключи связаны с конкретными пользователями.

Число неприводимых многочленов:\n
	l = 128, 256: (2^l - 2^{l/2}) / l;\n
	l = 192: (2^192 - 2^96 - 2^64 + 2^27) / l.

Таким образом, случайный многочлен степени l окажется неприводимым 
с вероятностью, близкой к 1/l. 

Если проверить k * l случайных многочленов, то ни один из них не окажется 
неприводимым с вероятностью, близкой к\n
	p = (1 - 1/l)^{k * l} \approx e^{-k}.\n
В реализации\n 
	k = B_PER_IMPOSSIBLE * 3 / 4 > B_PER_IMPOSSIBLE * \log(2)\n
и поэтому p < 2^{-B_PER_IMPOSSIBLE}.

В функции belsGenM0() неуспех при k * l попытках генерации интерпретируется 
как нарушение ожидаемых свойств ang с возвратом кода ошибки ERR_BAD_ANG.

Генерация открытых ключей пользователей основана на вспомогательном алгоритме 
BuildIrred, описанном в 6.3. В этом алгоритме входной многочлен u 
интерпретируется как элемент поля E = F_2[x]/(f0(x)), 
где f0(x) = x^l + m0(x) -- неприводимый многочлен степени l. 
Алгоритм BuildIrred возвращает минимальный многочлен f элемента u. 
Многочлен f либо неприводим, либо f == 1. Если deg f = l, 
то генерация завершена -- открытый ключ определяется по f. 

Каждый неприводимый многочлен степени l является минимальным многочленом 
для l различных элементов поля. Поэтому вероятность того, что f будет  
иметь степень l близка к\n
	(1 - 2^{-l/2}) [l = 128, 256]\n
	(1 - 2^{-96} - ...)	[l = 192].

В функциях belsGenMi(), belsGenMid() предпринимается\n
	k = \max(3, B_PER_IMPOSSIBLE * 2 / l)\n
попытки генерации f. Ситуация, когда все попытки завершены с ошибкой, 
считается невозможной и интерпретируется как неверные входные данные 
(общему открытому ключу m0 соответствует многочлен f0, 
который не является неприводимым) с возвратом кода ошибки ERR_BAD_PUBKEY.

\expect{ERR_BAD_INPUT} Все входные указатели действительны.

\safe todo
*******************************************************************************
*/

/*
*******************************************************************************
Открытые ключи
*******************************************************************************
*/

/*!	\brief Загрузка стандартного открытого ключа

	Загружается стандартный открытый ключ [len]m.
	Загружается либо общий открытый ключ из таблицы A.1 (при num == 0),
	либо открытый ключ пользователя номер num одной из таблиц A.2, A.3, A.4.
	\expect{ERR_BAD_INPUT}
	-	len == 16 || len == 24 || len == 32;
	-	0 <= num <= 16.
	.
	\return ERR_OK, если ключ успешно загружен, и код ошибки 
	в противном случае.
*/
err_t belsStdM(
	octet m[],			/*!< [out] открытый ключ */
	size_t len,			/*!< [in] длина ключа в октетах */
	size_t num			/*!< [in] номер ключа */
);

/*!	\brief Проверка открытого ключа

	Проверяется корректность открытого ключа [len]m.
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\return ERR_OK, если ключ корректен, и код ошибки в противном случае.
*/
err_t belsValM(
	const octet m[],	/*!< [in] открытый ключ */
	size_t len			/*!< [in] длина ключа в октетах */
);

/*!	\brief Генерация общего открытого ключа

	Генерируется общий открытый ключ [len]m0. При генерации 
	используется генератор ang и его состояние ang_state. 
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\expect{ERR_BAD_ANG} Генератор ang выдает неповторяющиеся ключи-кандидаты.
	\return ERR_OK, если ключ успешно сгенерирован, и код ошибки
	в противном случае.
	\remark Реализован алгоритм bels-genm0.
*/
err_t belsGenM0(
	octet m0[],				/*!< [out] общий открытый ключ */
	size_t len,				/*!< [in] длина ключа в октетах */
	gen_i ang,				/*!< [in] генератор произвольных чисел */
	void* ang_state			/*!< [in,out] состояние генератора */
);

/*!	\brief Генерация открытых ключей пользователей

	По общему открытому ключу [len]m0 октетов генерируется открытый ключ 
	[len]mi пользователя. При генерации используется генератор ang и его 
	состояние ang_state. 
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\expect{ERR_BAD_PUBKEY} Ключ m0 корректен.
	\expect{ERR_BAD_ANG} Генератор ang корректен и выдает неповторяющиеся 
	ключи-кандидаты.
	\return ERR_OK, если ключи успешно сгенерированы, и код ошибки
	в противном случае.
	\remark Частично реализован алгоритм bels-genmi. Для генерации набора
	открытых ключей следует вызвать функцию несколько раз и проверить различие
	полученных ключей. 
*/
err_t belsGenMi(
	octet mi[],				/*!< [out] открытый ключ пользователя */
	size_t len,				/*!< [in] длина ключей в октетах */
	const octet m0[],		/*!< [in] общий открытый ключ */
	gen_i ang,				/*!< [in] генератор произвольных чисел */
	void* ang_state			/*!< [in,out] состояние генератора */
);

/*!	\brief Генерация открытого ключа пользователя по идентификатору

	По общему открытому ключу [len]m0 и идентификатору [id_len]id октетов 
	генерируется открытый ключ [len]mid пользователя с идентификатором 
	[id_len]id.
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\expect{ERR_BAD_PUBKEY} Ключ m0 корректен.
	\return ERR_OK, если ключ успешно сгенерирован, и код ошибки
	в противном случае.
	\remark Реализован алгоритм genmid.
*/
err_t belsGenMid(
	octet mid[],			/*!< [out] открытый ключ пользователя */
	size_t len,				/*!< [in] длина ключей в октетах */
	const octet m0[],		/*!< [in] общий открытый ключ */
	const octet id[],		/*!< [in] идентификатор */
	size_t id_len			/*!< [in] длина идентификаторa в октетах */
);

/*
*******************************************************************************
Разделение и восстановление секрета
*******************************************************************************
*/

/*!	\brief Разделение секрета

	Секрет [len]s разделяется с порогом threshold на count частичных секретов, 
	которые записываются в массив [count * len]si. При разделении используется 
	общий открытый ключ [len]m0 и открытые ключи пользователей из массива 
	[count * len]mi. Секреты и ключи размещаются в массивах si, mi 
	последовательными блоками из len октетов: первый блок соответствует
	первому пользователю, второй -- второму и т.д. При разделении секрета 
	используется генератор rng и его состояние rng_state. 
	\expect{ERR_BAD_INPUT}
	-	len == 16 || len == 24 || len == 32;
	-	0 < threshold <= count.
	.
	\expect{ERR_BAD_PUBKEY} Открытые ключи m0, mi корректны и отличаются 
	друг от друга.
	\expect{ERR_BAD_RNG} Генератор rng (с состоянием rng_state) корректен.
	\expect Генератор rng является криптографически стойким.
	\return ERR_OK, если секрет успешно разделен, и код ошибки
	в противном случае.
	\remark Реализован алгоритм bels-share.
*/
err_t belsShare(
	octet si[],				/*!< [out] частичные секреты */
	size_t count,			/*!< [in] число пользователей */
	size_t threshold,		/*!< [in] пороговое число */
	size_t len,				/*!< [in] длина секрета в октетах */
	const octet s[],		/*!< [in] секрет */
	const octet m0[],		/*!< [in] общий открытый ключ */
	const octet mi[],		/*!< [in] открытые ключи пользователей */
	gen_i rng,				/*!< [in] генератор случайных чисел */
	void* rng_state			/*!< [in,out] состояние генератора */
);

/*!	\brief Разделение секрета на стандартных открытых ключах

	Секрет [len]s разделяется с порогом threshold на count частичных секретов,
	которые записываются в массив [count * (len + 1)]si. При разделении
	используются стандартные открытые ключи
		belsStdM(m, len, i), i = 0, 1, ..., count.
	Частичные секреты размещается в массиве si последовательными блоками из
	len + 1 октетов: первый блок соответствует первому пользователю, второй --
	второму и т.д. В первом октете блока указывается его номер (от 1 до count).
	При разделении секрета используется генератор rng и его состояние
	rng_state.
	\expect{ERR_BAD_INPUT}
	-	len == 16 || len == 24 || len == 32;
	-	0 < threshold <= count < = 16.
	.
	\expect{ERR_BAD_RNG} Генератор rng (с состоянием rng_state) корректен.
	\expect Генератор rng является криптографически стойким.
	\return ERR_OK, если секрет успешно разделен, и код ошибки в противном
	случае.
	\remark Реализован алгоритм bels-share.
*/
err_t belsShare2(
	octet si[],				/*!< [out] частичные секреты */
	size_t count,			/*!< [in] число пользователей */
	size_t threshold,		/*!< [in] пороговое число */
	size_t len,				/*!< [in] длина секрета в октетах */
	const octet s[],		/*!< [in] секрет */
	gen_i rng,				/*!< [in] генератор случайных чисел */
	void* rng_state			/*!< [in,out] состояние генератора */
);

/*!	\brief Детерминированное разделение секрета на стандартных открытых ключах

	Секрет [len]s разделяется с порогом threshold на count частичных секретов, 
	которые записываются в массив [count * (len + 1)]si. При разделении
	используется стандартные открытые ключи
		belsStdM(m, len, i), i = 0, 1, ..., count. 
	Секреты размещается в массиве si последовательными блоками из len + 1
	октетов. В первом октете блока указывается его номер (от 1 до count).
	Одноразовый ключ, который используется при разделении секрета, строится
	по разделяемому ключу с помощью (экспериментального) детерминированного
	алгоритма bels-genk.
	\expect{ERR_BAD_INPUT}
	-	len == 16 || len == 24 || len == 32;
	-	0 < threshold <= count <= 16.
	.
	\return ERR_OK, если секрет успешно разделен, и код ошибки в противном
	случае.
	\remark Реализован алгоритм bels-share.
	\warning Экспериментальный режим.
*/
err_t belsShare3(
	octet si[],				/*!< [out] частичные секреты */
	size_t count,			/*!< [in] число пользователей */
	size_t threshold,		/*!< [in] пороговое число */
	size_t len,				/*!< [in] длина секрета в октетах */
	const octet s[]			/*!< [in] секрет */
);

/*!	\brief Восстановление секрета

	Секрет [len]s восстанавливается по count частичным секретам из массива 
	[count * len]si. При восстановлении используется общий открытый ключ 
	[len]m0 и открытые ключи пользователей из массива [count * len]mi. 
	Частичные секреты и открытые ключи размещаются в массивах si, mi 
	последовательными блоками из len октетов.
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\expect{ERR_BAD_PUBKEY} Открытые ключи m0, mi корректны и отличаются 
	друг от друга.
	\return ERR_OK, если секрет успешно восстановлен, и код ошибки в противном 
	случае.
	\remark Реализован алгоритм bels-recover. Успешное восстановление не
	означает, что секрет будет совпадать с первоначально разделенным. Этого,
	например, не произойдет, если число частичных секретов меньше порога,
	использованого при разделении.
*/
err_t belsRecover(
	octet s[],				/*!< [out] восстановленный секрет */
	size_t count,			/*!< [in] число пользователей */
	size_t len,				/*!< [in] длина секретов в октетах */
	const octet si[],		/*!< [in] частичные секреты */
	const octet m0[],		/*!< [in] общий открытый ключ */
	const octet mi[]		/*!< [in] открытые ключи пользователей */
);

/*!	\brief Восстановление секрета на стандартных открытых ключах

	Секрет [len]s восстанавливается по count частичным секретам из массива
	[count * len]si. При восстановлении используется стандартные открытые
	ключи belsStdM(m, len, i). Частичные секреты размещаются в массиве si
	последовательными блоками из len + 1 октетов. Первый октет блока задает
	номер открытого ключа.
	\expect{ERR_BAD_INPUT} len == 16 || len == 24 || len == 32.
	\expect{ERR_BAD_PUBKEY} Номера открытых ключей, указанные в первых октетах
	частичных секретов, принадлежат интервалу {1, 2, ..., 16} и отличаются
	друг от друга.
	\return ERR_OK, если секрет успешно восстановлен, и код ошибки в противном
	случае.
	\remark Реализован алгоритм bels-recover. Успешное восстановление не
	означает, что секрет будет совпадать с первоначально разделенным. Этого,
	например, не произойдет, если число частичных секретов меньше порога,
	использованого при разделении.
*/
err_t belsRecover2(
	octet s[],				/*!< [out] восстановленный секрет */
	size_t count,			/*!< [in] число пользователей */
	size_t len,				/*!< [in] длина секретов в октетах */
	const octet si[]		/*!< [in] частичные секреты */
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __BEE2_BELS_H */
