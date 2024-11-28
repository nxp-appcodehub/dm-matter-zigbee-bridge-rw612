/*
 * Copyright 2021-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#include <assert.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "shell.h" 

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define KEY_ESC (0x1BU)
#define KET_DEL (0x7FU)

#define CMD_EVENT_QUEUE_ENTRY      3

typedef struct {
    const char   *p_cmd;
    TaskHandle_t  caller;
} CmdEvent_t;

typedef struct {
    printf_data_t printf_data_func;
} DummyContext_t;

#define BOOT_DELAY  5

#define CMD_HANDLER_TASK_STACK_SIZE             1024
#define CMD_HANDLER_TASK_PRIORITY               (tskIDLE_PRIORITY + 2)  

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int32_t HelpCommand(p_shell_context_t context, int32_t argc, char **argv); /*!< help command */

static int32_t ParseLine(const char *cmd, uint32_t len, char *argv[SHELL_MAX_ARGS]); /*!< parse line command */

static int32_t StrCompare(const char *str1, const char *str2, int32_t count); /*!< compare string command */

static int32_t ProcessCommand(p_shell_context_t context, const char *cmd); /*!< process a command */

static void GetHistoryCommand(p_shell_context_t context, uint8_t hist_pos); /*!< get commands history */

static void AutoComplete(p_shell_context_t context); /*!< auto complete command */

static uint8_t GetChar(p_shell_context_t context); /*!< get a char from communication interface */

static int32_t StrLen(const char *str); /*!< get string length */

static char *StrCopy(char *dest, const char *src, int32_t count); /*!< string copy */

static void CmdHandlerTask(void *p_param);

static void ScriptTask(void *p_param);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static const shell_command_context_t xHelpCommand = {"help", "\"help\":                 Lists all the registered commands\r\n",
                                                     HelpCommand, 0};

static shell_command_context_list_t g_RegisteredCommands;

static char g_paramBuffer[SHELL_BUFFER_SIZE];

static QueueHandle_t s_CmdQueue;

static TaskHandle_t s_ShellTaskHandler;
static TaskHandle_t s_CmdHandlerTaskHandler;

/*******************************************************************************
 * Code
 ******************************************************************************/
void shell_Init(
    p_shell_context_t context, send_data_cb_t send_cb, recv_data_cb_t recv_cb, printf_data_t shell_printf, char *prompt, char *end_prompt, const char *(*p_get_pwd)(void))
{
    assert(send_cb != NULL);
    assert(recv_cb != NULL);
    assert(prompt != NULL);
    assert(shell_printf != NULL);

    /* Memset for context */
    memset(context, 0, sizeof(shell_context_struct));
    context->send_data_func = send_cb;
    context->recv_data_func = recv_cb;
    context->printf_data_func = shell_printf;
    context->prompt = prompt;
    context->end_prompt = end_prompt;
    context->p_get_pwd = p_get_pwd;

 //   shell_RegisterCommand(&xHelpCommand);
}

int32_t SHELL_Main(p_shell_context_t context)
{
    uint8_t ch;
    int32_t i;
    BaseType_t rt;

    if (context == NULL)
    {
        return -1;
    }

    s_ShellTaskHandler = xTaskGetCurrentTaskHandle();
    s_CmdQueue = xQueueCreate(CMD_EVENT_QUEUE_ENTRY, sizeof(CmdEvent_t));
    vQueueAddToRegistry(s_CmdQueue, "CmdQueue");

    rt = xTaskCreate(CmdHandlerTask,
                    "CmdHandlerTask", 
                    CMD_HANDLER_TASK_STACK_SIZE, 
                    context,
                    CMD_HANDLER_TASK_PRIORITY,
                    &s_CmdHandlerTaskHandler);

    if (rt != pdPASS) {
 //       LOG(SHELL, ERR, "Create CmdHandlerTask failed, reason = %d\r\n", rt);
        return -1;
    }
    
    context->printf_data_func("\r\n\r\n\r\nMCU IoT Gateway (build: %s)\r\n", __DATE__);
    context->printf_data_func("Copyright (c) 2018 NXP Semiconductor\r\n\r\n");    
    
    bool interrupt = false;

#if 0 
    if (!interrupt) {

        rt = xTaskCreate(ScriptTask,
                        "ScriptTask", 
                        SCRIPT_TASK_STACK_SIZE, 
                        NULL,
                        SCRIPT_TASK_PRIORITY,
                        NULL);
        assert(rt == pdPASS);
        
        (void)xTaskNotifyWait(0, UINT32_MAX, NULL, portMAX_DELAY);
    }
#endif

    context->exit = false;

    context->printf_data_func("%s%s%s", context->prompt, context->p_get_pwd(), context->end_prompt);
    while (1)
    {
        if (context->exit)
        {
            break;
        }
        ch = GetChar(context);
        /* If error occured when getting a char, continue to receive a new char. */
        if ((uint8_t)(-1) == ch)
        {
            continue;
        }
        /* Special key */
        if (ch == KEY_ESC)
        {
            context->stat = kSHELL_Special;
            continue;
        }
        else if (context->stat == kSHELL_Special)
        {
            /* Function key */
            if (ch == '[')
            {
                context->stat = kSHELL_Function;
                continue;
            }
            context->stat = kSHELL_Normal;
        }
        else if (context->stat == kSHELL_Function)
        {
            context->stat = kSHELL_Normal;

            switch ((uint8_t)ch)
            {
                /* History operation here */
                case 'A': /* Up key */
                    GetHistoryCommand(context, context->hist_current);
                    if (context->hist_current < (context->hist_count - 1))
                    {
                        context->hist_current++;
                    }
                    break;
                case 'B': /* Down key */
                    GetHistoryCommand(context, context->hist_current);
                    if (context->hist_current > 0)
                    {
                        context->hist_current--;
                    }
                    break;
                case 'D': /* Left key */
                    if (context->c_pos)
                    {
                        context->printf_data_func("\b");
                        context->c_pos--;
                    }
                    break;
                case 'C': /* Right key */
                    if (context->c_pos < context->l_pos)
                    {
                        context->printf_data_func("%c", context->line[context->c_pos]);
                        context->c_pos++;
                    }
                    break;
                default:
                    break;
            }
            continue;
        }
        /* Handle tab key */
        else if (ch == '\t')
        {
#if SHELL_AUTO_COMPLETE
            /* Move the cursor to the beginning of line */
            for (i = 0; i < context->c_pos; i++)
            {
                context->printf_data_func("\b");
            }
            /* Do auto complete */
            AutoComplete(context);
            /* Move position to end */
            context->c_pos = context->l_pos = StrLen(context->line);
#endif
            continue;
        }
#if SHELL_SEARCH_IN_HIST
        /* Search command in history */
        else if ((ch == '`') && (context->l_pos == 0) && (context->line[0] == 0x00))
        {
        }
#endif
        /* Handle backspace key */
        else if ((ch == KET_DEL) || (ch == '\b'))
        {
            /* There must be at last one char */
            if (context->c_pos == 0)
            {
                continue;
            }

            context->l_pos--;
            context->c_pos--;

            if (context->l_pos > context->c_pos)
            {
                memmove(&context->line[context->c_pos], &context->line[context->c_pos + 1],
                        context->l_pos - context->c_pos);
                context->line[context->l_pos] = 0;
                context->printf_data_func("\b%s  \b", &context->line[context->c_pos]);

                /* Reset position */
                for (i = context->c_pos; i <= context->l_pos; i++)
                {
                    context->printf_data_func("\b");
                }
            }
            else /* Normal backspace operation */
            {
                context->printf_data_func("\b \b");
                context->line[context->l_pos] = 0;
            }
            continue;
        }
        else
        {
        }

        /* Input too long */
        if (context->l_pos >= (SHELL_BUFFER_SIZE - 1))
        {
            context->l_pos = 0;
        }

        /* Handle end of line, break */
        if ((ch == '\r') || (ch == '\n'))
        {
            static char endoflinechar = 0U;

            if ((endoflinechar != 0U) && (endoflinechar != ch))
            {
                continue;
            }
            else
            {
                endoflinechar = ch;
                context->printf_data_func("\r\n");
                /* If command line is NULL, will start a new transfer */
                if (0U == StrLen(context->line))
                {
                    context->printf_data_func("%s%s%s", context->prompt, context->p_get_pwd(), context->end_prompt);
                    continue;
                }

                (void)SHELL_InjectCmd(context->line);

                uint8_t tmpCommandLen = StrLen(context->line);
                /* Compare with last command. Push back to history buffer if different */
                if (tmpCommandLen != StrCompare(context->line, context->hist_buf[0], StrLen(context->line)))
                {
                    for (uint32_t i = SHELL_HIST_MAX - 1; i > 0; i--)
                    {
                        memset(context->hist_buf[i], '\0', SHELL_BUFFER_SIZE);
                        tmpCommandLen = StrLen(context->hist_buf[i - 1]);
                        StrCopy(context->hist_buf[i], context->hist_buf[i - 1], tmpCommandLen);
                    }
                    memset(context->hist_buf[0], '\0', SHELL_BUFFER_SIZE);
                    tmpCommandLen = StrLen(context->line);
                    StrCopy(context->hist_buf[0], context->line, tmpCommandLen);
                    if (context->hist_count < SHELL_HIST_MAX)
                    {
                        context->hist_count++;
                    }
                }

                /* Reset all params */
                context->c_pos = context->l_pos = 0;
                context->hist_current = 0;
                context->printf_data_func("%s%s%s", context->prompt, context->p_get_pwd(), context->end_prompt);
                memset(context->line, 0, sizeof(context->line));
                continue;
            }
        }

        /* Normal character */
        if (context->c_pos < context->l_pos)
        {
            memmove(&context->line[context->c_pos + 1], &context->line[context->c_pos],
                    context->l_pos - context->c_pos);
            context->line[context->c_pos] = ch;
            context->printf_data_func("%s", &context->line[context->c_pos]);
            /* Move the cursor to new position */
            for (i = context->c_pos; i < context->l_pos; i++)
            {
                context->printf_data_func("\b");
            }
        }
        else
        {
            context->line[context->l_pos] = ch;
            context->printf_data_func("%c", ch);
        }

        ch = 0;
        context->l_pos++;
        context->c_pos++;
    }
    return 0;
}

static int32_t HelpCommand(p_shell_context_t context, int32_t argc, char **argv)
{
    uint8_t i = 0;

    for (i = 0; i < g_RegisteredCommands.numberOfCommandInList; i++)
    {
        context->printf_data_func(g_RegisteredCommands.CommandList[i]->pcHelpString);
    }
    return 0;
}

static int32_t ProcessCommand(p_shell_context_t context, const char *cmd)
{
    const shell_command_context_t *tmpCommand = NULL;
    const char *tmpCommandString              = NULL;
    uint8_t tmpCommandLen                     = 0;
    bool is_param_valid                       = false;
    bool is_cmd_found                         = false;
    int32_t argc = 0;
    char *argv[SHELL_MAX_ARGS];
    int32_t rt = -1;

    tmpCommandLen = StrLen(cmd);
    argc = ParseLine(cmd, tmpCommandLen, argv);

    if (argc > 0)
    {
        for (uint32_t i = 0; i < g_RegisteredCommands.numberOfCommandInList; i++)
        {
            tmpCommand = g_RegisteredCommands.CommandList[i];
            tmpCommandString = tmpCommand->pcCommand;
            tmpCommandLen = StrLen(tmpCommandString);

            if (StrCompare(tmpCommandString, argv[0], tmpCommandLen) == 0)
            {
                is_cmd_found = true;
                
                /* support commands with optional number of parameters */
                if (tmpCommand->cExpectedNumberOfParameters == SHELL_OPTIONAL_PARAMS)
                {
                    is_param_valid = true;
                }
                else if ((tmpCommand->cExpectedNumberOfParameters == 0) && (argc == 1))
                {
                    is_param_valid = true;
                }
                else if (tmpCommand->cExpectedNumberOfParameters > 0)
                {
                    if ((argc - 1) == tmpCommand->cExpectedNumberOfParameters)
                    {
                        is_param_valid = true;
                    }
                }
                
                break;
            }
        }

        if (is_cmd_found) {
            if (is_param_valid) {
                
                rt = tmpCommand->pFuncCallBack(context, argc, argv);

            } else {
                context->printf_data_func("Error: Incorrect command or parameters\r\n");
            }
        } else {
            context->printf_data_func("Command not recognised. Enter 'help' to view a list of available commands.\r\n");
        }
    }
    
    return rt;
}

static void GetHistoryCommand(p_shell_context_t context, uint8_t hist_pos)
{
    uint8_t i;
    uint32_t tmp;

    if (context->hist_buf[0][0] == '\0')
    {
        context->hist_current = 0;
        return;
    }
    if (hist_pos >= SHELL_HIST_MAX)
    {
        hist_pos = SHELL_HIST_MAX - 1;
    }
    tmp = StrLen(context->line);
    /* Clear current if have */
    if (tmp > 0)
    {
        memset(context->line, '\0', tmp);
        for (i = 0; i < tmp; i++)
        {
            context->printf_data_func("\b \b");
        }
    }

    context->l_pos = StrLen(context->hist_buf[hist_pos]);
    context->c_pos = context->l_pos;
    StrCopy(context->line, context->hist_buf[hist_pos], context->l_pos);
    context->printf_data_func(context->hist_buf[hist_pos]);
}

static void AutoComplete(p_shell_context_t context)
{
    int32_t len;
    int32_t minLen;
    uint8_t i = 0;
    const shell_command_context_t *tmpCommand = NULL;
    const char *namePtr;
    const char *cmdName;

    minLen = 0;
    namePtr = NULL;

    if (!StrLen(context->line))
    {
        return;
    }
    context->printf_data_func("\r\n");
    /* Empty tab, list all commands */
    if (context->line[0] == '\0')
    {
        HelpCommand(context, 0, NULL);
        return;
    }
    /* Do auto complete */
    for (i = 0; i < g_RegisteredCommands.numberOfCommandInList; i++)
    {
        tmpCommand = g_RegisteredCommands.CommandList[i];
        cmdName = tmpCommand->pcCommand;
        if (StrCompare(context->line, cmdName, StrLen(context->line)) == 0)
        {
            if (minLen == 0)
            {
                namePtr = cmdName;
                minLen = StrLen(namePtr);
                /* Show possible matches */
                context->printf_data_func("%s\r\n", cmdName);
                continue;
            }
            len = StrCompare(namePtr, cmdName, StrLen(namePtr));
            if (len < 0)
            {
                len = len * (-1);
            }
            if (len < minLen)
            {
                minLen = len;
            }
        }
    }
    /* Auto complete string */
    if (namePtr)
    {
        StrCopy(context->line, namePtr, minLen);
    }
    context->printf_data_func("%s%s%s%s", context->prompt, context->p_get_pwd(), context->end_prompt, context->line);
    return;
}

static char *StrCopy(char *dest, const char *src, int32_t count)
{
    char *ret = dest;
    int32_t i = 0;

    for (i = 0; i < count; i++)
    {
        dest[i] = src[i];
    }

    return ret;
}

static int32_t StrLen(const char *str)
{
    int32_t i = 0;

    while (*str)
    {
        str++;
        i++;
    }
    return i;
}

static int32_t StrCompare(const char *str1, const char *str2, int32_t count)
{
    while (count--)
    {
        if (*str1++ != *str2++)
        {
            return *(unsigned char *)(str1 - 1) - *(unsigned char *)(str2 - 1);
        }
    }
    return 0;
}

static int32_t ParseLine(const char *cmd, uint32_t len, char *argv[SHELL_MAX_ARGS])
{
    uint32_t argc;
    char *p;
    uint32_t position;

    /* Init params */
    memset(g_paramBuffer, '\0', len + 1);
    StrCopy(g_paramBuffer, cmd, len);

    p = g_paramBuffer;
    position = 0;
    argc = 0;

    while (position < len)
    {
        /* Skip all blanks */
        while (((char)(*p) == ' ') && (position < len))
        {
            *p = '\0';
            p++;
            position++;
        }
        /* Process begin of a string */
        if (*p == '"')
        {
            p++;
            position++;
            argv[argc] = p;
            argc++;
            /* Skip this string */
            while ((*p != '"') && (position < len))
            {
                p++;
                position++;
            }
            /* Skip '"' */
            *p = '\0';
            p++;
            position++;
        }
        else /* Normal char */
        {
            argv[argc] = p;
            argc++;
            while (((char)*p != ' ') && ((char)*p != '\t') && (position < len))
            {
                p++;
                position++;
            }
        }
    }
    return argc;
}

int32_t shell_RegisterCommand(const shell_command_context_t *command_context) //API name changed 
{
    int32_t result = 0;

    /* If have room  in command list */
    if (g_RegisteredCommands.numberOfCommandInList < SHELL_MAX_CMD)
    {
        g_RegisteredCommands.CommandList[g_RegisteredCommands.numberOfCommandInList++] = command_context;
    }
    else
    {
        result = -1;
    }
    return result;
}

int32_t dummy_printf( const char *pcFormat, ... )
{
    return 0;
}

static uint8_t GetChar(p_shell_context_t context)
{
    uint8_t ch;

#if SHELL_USE_FILE_STREAM
    ch = fgetc(context->STDIN);
#else
    (void)context->recv_data_func(&ch, 1U, portMAX_DELAY);
#endif
    return ch;
}

static void CmdHandlerTask(void *p_param)
{
    BaseType_t api_rt;
    CmdEvent_t event;
    DummyContext_t dummy;
    p_shell_context_t p_context;
    int32_t rt;

    while (1) {
        api_rt = xQueueReceive(s_CmdQueue, &event, portMAX_DELAY);
        if (api_rt == pdPASS) {
            
            if (event.caller == s_ShellTaskHandler) {
                p_context = (p_shell_context_t)p_param;
            } else {
                dummy.printf_data_func = dummy_printf;
                p_context = (p_shell_context_t)&dummy;
            }

            rt = ProcessCommand(p_context, event.p_cmd);
            /*
                command success: 0
                command fails: -1 (convert to MAX in uint32_t)
            */
            api_rt = xTaskNotify(event.caller, (uint32_t)rt, eSetValueWithOverwrite);
            if (api_rt != pdPASS) {
            //    LOG(SHELL, WARN, "Notify caller task failed, reason = %d\r\n", api_rt);
            }
        } else {
         //   LOG(SHELL, WARN, "Wait command queue failed, reason = %d\r\n", api_rt);
        }
    }
}

int32_t SHELL_InjectCmd(const char *p_cmd) 
{
    BaseType_t api_rt;
    int32_t    rt = -1;
    CmdEvent_t cmdevent;
    uint32_t ulNotifiedValue;

    cmdevent.p_cmd  = p_cmd;
    cmdevent.caller = xTaskGetCurrentTaskHandle();

    /* The api should not be call from shell command function which is running in the same task context */
    assert(cmdevent.caller != s_CmdHandlerTaskHandler);
    
    api_rt = xQueueSendToBack(s_CmdQueue, &cmdevent, 0);
    if (api_rt == pdPASS) {
        
        api_rt = xTaskNotifyWait(0, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);
        if (api_rt == pdPASS) {
            rt = (int32_t)ulNotifiedValue;
        } else {
         //   LOG(SHELL, WARN, "Wait command result notification failed, reason = %d\r\n", api_rt);
        }
    } else {
       // LOG(SHELL, WARN, "Send command to handler task failed, reason = %d\r\n", api_rt);
    }
    
    return rt;
}

#if 0
char* lfs_file_gets(lfs_t *p_lfs, void *p_buffer, lfs_size_t size, lfs_file_t *p_file)
{
	uint32_t nc = 0;
	char *p = p_buffer;
	char ch;
	lfs_ssize_t rc;
    
	while (nc < size - 1) {
		rc = lfs_file_read(p_lfs, p_file, &ch, 1);
		if (rc != 1) break;
		if (ch == '\r') continue;
        if (ch == '\n') break;
		*p++ = ch; 
        nc++;
	}
    
	*p = '\0';
    
	return (nc > 0) ? p_buffer : NULL;
}

static void ScriptTask(void *p_param)
{
    int32_t rt;
    lfs_file_t file;
    lfs_t lfs;
    
    rt = lfs_mount(&lfs, &g_lfsc_default);
    if (rt != 0) {
        rt = lfs_format(&lfs, &g_lfsc_default);
        assert(rt == 0);
        
        rt = lfs_mount(&lfs, &g_lfsc_default);
        assert(rt == 0);   
    }
    
    rt = lfs_mkdir(&lfs, "/flash");
    assert((rt == 0) || (rt == LFS_ERR_EXIST));
    
    rt = lfs_file_open(&lfs, &file, "/flash/boot", LFS_O_RDONLY);
    if (rt == 0) {
        
        char *p_container = pvPortMalloc(SHELL_BUFFER_SIZE);
        char *p_cmd  = NULL;
        
        uint32_t line = 0;

        do {
            p_cmd = lfs_file_gets(&lfs, p_container, SHELL_BUFFER_SIZE, &file);
            if (p_cmd == NULL) {
                break;
            }
            line++;
            rt = SHELL_InjectCmd(p_cmd);
            if (rt != 0) {
            //    LOG(SHELL, WARN, "Script command exec failed on %d line\r\n", line);
                break;
            }
        } while (true);
            
        vPortFree(p_container);
        
        rt = lfs_file_close(&lfs, &file);
        assert(rt == 0);
    }
    
    rt = lfs_unmount(&lfs);
    assert(rt == 0);
    
    /* end of script execusion */
    xTaskNotifyGive(s_ShellTaskHandler);
    
    vTaskDelete(NULL);
}
#endif
