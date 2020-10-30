/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */


#include <linux/delay.h>

#include "nt36xxx.h"

int nvt_get_fw_info(void);


static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "NO%02X%02X%02X%02X",
		ts->fw_ver_bin[0], ts->fw_ver_bin[1], ts->fw_ver_bin[2], ts->fw_ver_bin[3]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");

	NVT_LOG("%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char data[4] = { 0 };
	char model[16] = { 0 };
	int ret;

	sec_cmd_set_default_result(sec);

	if (ts->power_status == POWER_OFF_STATUS) {
		NVT_ERR("%s: POWER_STATUS : OFF!\n", __func__);
		goto out_power_off;
	}

	/* ic_name, project_id, panel, fw info */
	ret = nvt_get_fw_info();
	if (ret < 0) {
		NVT_ERR("%s: nvt_ts_get_fw_info fail!\n", __func__);
		goto out;
	}
	data[0] = ts->fw_ver_ic[0];
	data[1] = ts->fw_ver_ic[1];
	data[2] = ts->fw_ver_ic[2];
	data[3] = ts->fw_ver_ic[3];

	snprintf(buff, sizeof(buff), "NO%02X%02X%02X%02X", data[0], data[1], data[2], data[3]);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;

	snprintf(model, sizeof(model), "NO%02X%02X", data[0], data[1]);

	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
		sec_cmd_set_cmd_result_all(sec, model, strnlen(model, sizeof(model)), "FW_MODEL");
	}

	NVT_LOG("%s: %s\n", __func__, buff);

	return;

out:
	nvt_bootloader_reset();
out_power_off:
	snprintf(buff, sizeof(buff), "%s", "NG");
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	NVT_LOG("%s: %s\n", __func__, buff);
}


static void aot_enable(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct nvt_ts_data *info = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		if (sec->cmd_param[0] == 0) {
			info->gesture_mode = 0;
		} else {
			info->gesture_mode = 1;
		}
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void glove_mode(void *device_data)
{

}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
//	struct nvt_ts_data *ts = container_of(sec, struct nvt_ts_data, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	NVT_LOG("%s: %s\n", __func__, buff);
}

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("aot_enable", aot_enable),},
	{SEC_CMD("glove_mode", glove_mode),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

int nvt_ts_sec_fn_init(struct nvt_ts_data *ts)
{
	int ret;

	ret = sec_cmd_init(&ts->sec, sec_cmds, ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_TSP);
	if (ret < 0) {
		NVT_ERR("%s: failed to sec cmd init\n",
			__func__);
		return ret;
	}

	ret = sysfs_create_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");
	if (ret) {
		NVT_ERR("%s: failed to creat sysfs link\n",
			__func__);
		goto out;
	}

	NVT_LOG("%s\n", __func__);

	return 0;

out:
	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);

	return ret;
}

void nvt_ts_sec_fn_remove(struct nvt_ts_data *ts)
{
	sysfs_delete_link(&ts->sec.fac_dev->kobj, &ts->input_dev->dev.kobj, "input");
	sec_cmd_exit(&ts->sec, SEC_CLASS_DEVT_TSP);
}
