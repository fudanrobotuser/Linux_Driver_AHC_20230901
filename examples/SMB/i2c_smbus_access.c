#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */

#define u8	__u8
#define u16	__u16
#define u32	__u32
#define s8	__s8
#define s16	__s16
#define s32	__s32

#define CHECK_I2C_FUNC( var, label )\
	do { 	if(0 == (var & label)) {\
		printf("\nError: "\
			#label " function is required. Program halted.\n\n");\
		}\
	} while(0);

static int i2c_smbus_access(
	int file,
	u8 read_write,
	u8 command,
	u32 size,
	union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data smbusdata;

	smbusdata.read_write = read_write;
	smbusdata.command = command;
	smbusdata.size = size;
	smbusdata.data = data;
	return ioctl(file, I2C_SMBUS, &smbusdata);
}

static int i2c_smbus_quick(
	int file,
	u8 read_write)/*read:1 write:0*/
{
	return i2c_smbus_access(file, read_write, 0, I2C_SMBUS_QUICK, NULL);
}

static int i2c_smbus_write_byte(
	int file,
	u8 command)
{
	return i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE, NULL);
}

static int i2c_smbus_read_byte(
	int file)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data))
		return -1;
	else
		return 0x0FF & data.byte;
}

static int i2c_smbus_write_byte_data(
	int file,
	u8 command,
	u8 byte)
{
	union i2c_smbus_data data;
	data.byte = byte;
	return i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
}

static int i2c_smbus_read_byte_data(
	int file,
	u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,
		I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data))
		return -1;
	else
		return 0xFF & data.byte;
}

static int i2c_smbus_write_word_data(
	int file,
	u8 command,
	u16 word)
{
	union i2c_smbus_data data;
	data.word = word;
	return i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
}

static int i2c_smbus_read_word_data(
	int file,
	u8 command)
{
	union i2c_smbus_data data;
	if (i2c_smbus_access(file,
		I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data))
		return -1;
	else
		return 0xFFFF & data.word;
}

static int i2c_smbus_process_call(
	int file,
	u8 command,
	u16 word)
{
	union i2c_smbus_data data;
	data.word = word;
	if (i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_PROC_CALL, &data))
		return -1;
	else
		return 0xFFFF & data.word;
}

static int i2c_smbus_write_block_data(
	int file,
	u8 command,
	u8 length,
	u8 *pbytes)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	data.block[0] = length;
	for (i = 1; i <= length; i++)
		data.block[i] = pbytes[i-1];
	return i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_BLOCK_DATA, &data);
}

static int i2c_smbus_read_block_data(
	int file,
	u8 command,
	u8 *pbytes)
{
	union i2c_smbus_data data;
	int i;
	if (i2c_smbus_access(file,
		I2C_SMBUS_READ, command, I2C_SMBUS_BLOCK_DATA, &data))
		return -1;
	else {
		/**plength = data.block[0];*/
		for (i = 1; i <= data.block[0]; i++)
			pbytes[i-1] = data.block[i];
		return data.block[0];
	}
}

static int i2c_smbus_block_process_call(
	int file,
	u8 command,
	u8 length,
	u8 *pbytes)
{
	union i2c_smbus_data data;
	int i;
	if (length > 32)
		length = 32;
	data.block[0] = length;
	for (i = 1; i <= length; i++)
		data.block[i] = pbytes[i-1];
	if (i2c_smbus_access(file,
		I2C_SMBUS_WRITE, command, I2C_SMBUS_BLOCK_PROC_CALL, &data))
		return -1;
	else {
		/**plength = data.block[0];*/
		for (i = 1; i <= data.block[0]; i++)
			pbytes[i-1] = data.block[i];
		return data.block[0];
	}
}

int main(int argc, char** argv)
{
	int result;
	int i;

	u32 busid;
	char devicename[64];

	int file_tty;
	unsigned long funcs;
	int deviceaddress;
	u8 blockdata[32];

	file_tty = -1;

	if (argc < 3) {
		printf("exe BusID DeviceAddress\n");
		goto funcexit;
	} else {
		busid = atoi(argv[1]);
		deviceaddress = atoi(argv[2]);
	}

	sprintf(devicename, "/dev/i2c-%u", busid);
	file_tty = open(devicename, O_RDWR);
	if (file_tty < 0) {
		printf("%s can not find \n", devicename);
		file_tty = -1;
		goto funcexit;
	}
	result = ioctl(file_tty, I2C_FUNCS, &funcs);
	if (result < 0) {
		printf("    can not ioctl I2C_FUNCS \n");
		goto funcexit;
	}
	/*check for funcs*/
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_WORD_DATA);
	CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_WORD_DATA);

	deviceaddress = 0x50; //  bit: 1 0 1 0 A2 A1 A0
	result = ioctl(file_tty, I2C_SLAVE, deviceaddress);
	if (result < 0) {
		printf("    can not ioctl I2C_SLAVE \n");
		goto funcexit;
	}

	result = ioctl(file_tty, I2C_PEC, 1);/*default 0:disable 1:enable*/
	if (result < 0) {
		printf("    can not ioctl I2C_PEC \n");
		goto funcexit;
	}
	
	for (i = 1; i <= 32; i++)
		blockdata[i] = i;

	result = i2c_smbus_quick(file_tty, 1);
	printf("    i2c_smbus_quick result:%i\n", result);
	
	result = i2c_smbus_write_byte(file_tty, 0xcc);
	printf("    i2c_smbus_write_byte result:%i\n", result);
	result = i2c_smbus_read_byte(file_tty);
	if (result < 0)
		printf("    i2c_smbus_read_byte result:%i\n", result);
	else
		printf("    i2c_smbus_read_byte byte:x%02x\n", result);
	
	result = i2c_smbus_write_byte_data(file_tty, 0xcc, 0xbb);
	printf("    i2c_smbus_write_byte_data result:%i\n", result);
	result = i2c_smbus_read_byte_data(file_tty, 0xcc);
	if (result < 0)
		printf("    i2c_smbus_read_byte_data result:%i\n", result);
	else
		printf("    i2c_smbus_read_byte_data byte:x%02x\n", result);
	
	result = i2c_smbus_write_word_data(file_tty, 0xcc, 0xdddd);
	printf("    i2c_smbus_write_word_data result:%i\n", result);
	result = i2c_smbus_read_word_data(file_tty, 0xcc);
	if (result < 0)
		printf("    i2c_smbus_read_word_data result:%i\n", result);
	else
		printf("    i2c_smbus_read_word_data word:x%04x\n", result);
	result = i2c_smbus_process_call(file_tty, 0xcc, 0xdddd);
	if (result < 0)
		printf("    i2c_smbus_process_call result:%i\n", result);
	else
		printf("    i2c_smbus_process_call word:x%04x\n", result);
	
	result = i2c_smbus_write_block_data(file_tty, 0xcc, 32, blockdata);
	printf("    i2c_smbus_write_block_data result:%i\n", result);
	result = i2c_smbus_read_block_data(file_tty, 0xcc, blockdata);
	if (result < 0)
		printf("    i2c_smbus_read_block_data result:%i\n", result);
	else
		printf("    i2c_smbus_read_block_data length:%i\n", result);
	result = i2c_smbus_block_process_call(file_tty, 0xcc, 32, blockdata);
	if (result < 0)
		printf("    i2c_smbus_block_process_call result:%i\n", result);
	else
		printf("    i2c_smbus_block_process_call length:%i\n", result);

	close(file_tty);
	file_tty = -1;

	goto funcexit;
funcexit:

	if (file_tty != -1) {
		close(file_tty);
		file_tty = -1;
	}

	return 0;
}
