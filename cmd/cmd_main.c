/*
*******************************************************************************
\file cmd_main.c
\brief Command-line interface to Bee2: main
\project bee2/cmd
\created 2022.06.07
\version 2022.06.10
\license This program is released under the GNU General Public License 
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

#include "cmd.h"
#include <bee2/core/err.h>
#include <bee2/core/str.h>
#include <bee2/core/util.h>
#include <stdio.h>

/*
*******************************************************************************
Лого
*******************************************************************************
*/

void cmdLogo()
{
	printf(
		"bee2cmd: Command-line interface to Bee2 [v%s]\n",
		utilVersion());
}

/*
*******************************************************************************
Регистрация команд
*******************************************************************************
*/

typedef struct
{
	const char* name;	/*!< имя команды */
	const char* descr;	/*!< описание команды */
	cmd_main_i fn;		/*!< точка входа команды */
} cmd_entry_t;

static size_t _count = 0;	/*< число команд */
static cmd_entry_t _cmds[32];	/*< перечень команд */

err_t cmdReg(const char* name, const char* descr, cmd_main_i fn)
{
	size_t pos;
	// команда уже зарегистрирована?
	for (pos = 0; pos < _count; ++pos)
		if (strEq(_cmds[pos].name, name))
			return ERR_CMD_EXISTS;
	// нет места?
	if (pos == COUNT_OF(_cmds))
		return ERR_OUTOFMEMORY;
	// зарегистрировать
	_cmds[pos].name = name;
	_cmds[pos].descr = descr;
	_cmds[pos].fn = fn;
	++_count;
	return ERR_OK;
}

/*
*******************************************************************************
Справка
*******************************************************************************
*/

int cmdUsage()
{
	size_t pos;
	// перечень команд
	printf(
		"Usage:\n"
		"  bee2cmd {");
	for (pos = 0; pos + 1 < _count; ++pos)
		printf("%s|", _cmds[pos].name);
	printf("%s} ...\n", _cmds[pos].name);
	// краткая справка по каждой команде
	for (pos = 0; pos < _count; ++pos)
		printf("    %s:\t%s\n", _cmds[pos].name, _cmds[pos].descr);
	return -1;
}

/*
*******************************************************************************
Инициализация
*******************************************************************************
*/

extern err_t bsumInit();

err_t cmdInit()
{
	err_t code;
	code = bsumInit();
	ERR_CALL_CHECK(code);
	return code;
}

/*
*******************************************************************************
Главная функция
*******************************************************************************
*/

int main(int argc, char* argv[])
{
	err_t code;
	size_t pos;
	// старт
	cmdLogo();
	code = cmdInit();
	if (code != ERR_OK)
	{
		printf("bee2cmd: %s\n", errMsg(code));
		return -1;
	}
	// справка
	if (argc < 2)
		return cmdUsage();
	// вызов команды
	for (pos = 0; pos < COUNT_OF(_cmds); ++pos)
		if (strEq(argv[1], _cmds[pos].name))
			return _cmds[pos].fn(argc - 1,  argv + 1);
	printf("bee2cmd: %s\n", errMsg(ERR_CMD_NOT_FOUND));
	return -1;
}
