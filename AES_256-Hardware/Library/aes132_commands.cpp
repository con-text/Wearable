// ----------------------------------------------------------------------------
//         ATMEL Crypto-Devices Software Support  -  Colorado Springs, CO -
// ----------------------------------------------------------------------------
// DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
// DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

/** \file
 *  \brief  This file contains implementations of command functions
 *          for the AES132 device.
 *  \author Atmel Crypto Products
 *  \date   June 13, 2011
 */
#include "Arduino.h"
#include "aes132_commands.h"
#include "aes132_i2c.h"

/** \brief This function sends an Info command and reads its response.
 *
 * @param[in] selector
 *        0x00: Return the current MacCount value. This value may be useful
 *              if the tag and system get out of sync.
 *              0x00 is always returned as the most significant byte of the
 *              Result field.
 *        0x05: Returns the KeyID if a previous Auth command succeeded,
 *              otherwise returns 0xFFFF. The KeyID is reported as 0x00KK,
 *              where KK is the KeyID number.
 *        0x06: The first byte provides the Atmel device code which can be
 *              mapped to an Atmel catalog number. The second byte provides
 *              the device revision number.
 *        0x0C: Returns a code indicating the device state:
                   0x0000 indicates the ChipState = Active.
                   0xFFFF indicates the ChipState =  Power Up.
                   0x5555 indicates the ChipState = "Wakeup from Sleep".
 * @param[out] result pointer to response
 * @return status of the operation
 */
uint8_t aes132m_info(uint8_t selector, uint8_t *result)
{
	uint8_t command[AES132_COMMAND_SIZE_MIN] = {AES132_COMMAND_SIZE_MIN, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	command[AES132_COMMAND_INDEX_PARAM1_LSB] = selector;

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_INFO, result, AES132_OPTION_DEFAULT);
}


/** \brief This function sends a TempSense command, reads the response, and returns the
 *         difference between the high and low temperature values.
 * @param temp_diff difference between the high and low temperature values inside response
 * @return status of the operation
 */
uint8_t aes132m_temp_sense(uint16_t *temp_diff)
{
	const uint8_t command[] = {AES132_COMMAND_SIZE_MIN, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD9, 0x9C};
	uint8_t response[AES132_RESPONSE_SIZE_TEMP_SENSE];

	uint8_t aes132_lib_return = aes132c_send_and_receive((uint8_t *) command,
				AES132_RESPONSE_SIZE_TEMP_SENSE, response, AES132_OPTION_NO_APPEND_CRC);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS)
		return aes132_lib_return;

	// Calculate temperature difference.
	*temp_diff = (uint16_t) ((response[AES132_RESPONSE_INDEX_TEMP_CODE_HIGH_MSB] * 256 + response[AES132_RESPONSE_INDEX_TEMP_CODE_HIGH_LSB])
	                       - (response[AES132_RESPONSE_INDEX_TEMP_CODE_LOW_MSB] * 256 + response[AES132_RESPONSE_INDEX_TEMP_CODE_LOW_LSB]));
	return aes132_lib_return;
}


/** This function sends a BlockRead command and receives the read data.
 *
 * @param[in] word_address start address to read from
 * @param[in] n_bytes number of bytes to read
 * @param[out] result pointer to read buffer
 * @return status of the operation
 */
uint8_t aes132m_block_read(uint16_t word_address, uint8_t n_bytes, uint8_t *result)
{
	uint8_t command[AES132_COMMAND_SIZE_MIN] = {AES132_COMMAND_SIZE_MIN, AES132_OPCODE_BLOCK_READ, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	command[AES132_COMMAND_INDEX_PARAM1_MSB] = word_address >> 8;
	command[AES132_COMMAND_INDEX_PARAM1_LSB] = word_address & 0xFF;
	command[AES132_COMMAND_INDEX_PARAM2_LSB] = n_bytes;

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_BLOCK_READ + n_bytes, result, AES132_OPTION_DEFAULT);
}


/** This generates a random number
 */
uint8_t randomNumber(uint8_t *result)
{
	byte mode = B00000010;
	uint8_t command[AES132_COMMAND_SIZE_MIN] = {AES132_COMMAND_SIZE_MIN, AES132_OPCODE_RANDOM, mode, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_MIN + 16, result, AES132_OPTION_DEFAULT);
}

/** This writes a key to the right area in memory */
uint8_t writeKey(uint8_t *key, uint16_t keyID)
{
	uint16_t address = 0xF200;
	// First write the key to memory
	aes132c_write_memory(16, address, key);

	// Write the key config
	byte keyConfig[4];
	keyConfig[0] = B00001001;
	keyConfig[1] = B11011000;
	keyConfig[2] = B00000000;
	keyConfig[3] = B00000000;
	address = 0xF080;
	aes132c_write_memory(sizeof(keyConfig), address, (uint8_t*)&keyConfig);
}

uint8_t encrypt(uint8_t dataToEncrypt[16], uint8_t *result)
{
	byte mode = B00000000;

	uint8_t command[AES132_COMMAND_SIZE_MIN+16] = {AES132_COMMAND_SIZE_MIN+16, AES132_OPCODE_ENCRYPT, mode, 0x00, 0x00, 0x00, 0x00, 
													//data
													0x00, 0x00, 0x00, 0x00, 
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													// checksums
													0x00, 0x00};

	// Copy the data into the right place
	memcpy(&command[7], dataToEncrypt, 16*sizeof(uint8_t));
	// Set the key to use
	command[AES132_COMMAND_INDEX_PARAM1_LSB] = 0x01;
	// Set the size of the data packet to be decrypted
	command[AES132_COMMAND_INDEX_PARAM2_LSB] = 0x10;

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_MIN + 32, result, AES132_OPTION_DEFAULT);
}

uint8_t decrypt(uint8_t *inMac, uint8_t *dataToDecrypt, uint8_t *result)
{
	byte mode = B00000000;

	// 16 bytes for the inMac, 16 bytes for the input data
	uint8_t command[AES132_COMMAND_SIZE_MIN+32] = {AES132_COMMAND_SIZE_MIN+32, AES132_OPCODE_DECRYPT, mode, 0x00, 0x00, 0x00, 0x00, 
													//inMac
													0x00, 0x00, 0x00, 0x00, 
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													// data
													0x00, 0x00, 0x00, 0x00, 
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													// Checksums
													0x00, 0x00};

	// Set the key to use
	command[AES132_COMMAND_INDEX_PARAM1_LSB] = 0x01;
	// Set the size of the data packet to be return
	command[AES132_COMMAND_INDEX_PARAM2_LSB] = 0x10;

	// Copy inMac into the right place
	memcpy(&command[7], inMac, 16*sizeof(uint8_t));

	// Copy data into the right place
	memcpy(&command[23], dataToDecrypt, 16*sizeof(uint8_t));

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_MIN+16, result, AES132_OPTION_DEFAULT);

}

uint8_t nonce(uint8_t *result) 
{
	byte mode = B00000000;

	uint8_t command[AES132_COMMAND_SIZE_MIN+12] = {AES132_COMMAND_SIZE_MIN+12, AES132_OPCODE_NONCE, mode, 0x00, 0x00, 0x00, 0x00, 
													0x00, 0x00, 0x00, 0x00, 
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00, 0x00, 0x00,
													0x00, 0x00};

	return aes132c_send_and_receive(command, AES132_RESPONSE_SIZE_MIN, result, AES132_OPTION_DEFAULT);
}