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
 *  \brief 	This file contains implementations of I2C functions.
 *  \author Atmel Crypto Products
 *  \date 	June 16, 2011
 */

#include "Arduino.h"
#include <Wire.h>
#include "aes132_i2c.h"                //!< I2C library definitions
#include "aes132_commands.h"

/** \brief I2C address used at AES132 library startup. */
#define AES132_I2C_DEFAULT_ADDRESS   ((uint8_t) 0x50)


/** \brief These enumerations are flags for I2C read or write addressing. */
enum aes132_i2c_read_write_flag {
	I2C_WRITE = (uint8_t) 0x00,	//!< write command id
	I2C_READ  = (uint8_t) 0x01   //! read command id
};

//! I2C address currently in use.
static uint8_t i2c_address_current = AES132_I2C_DEFAULT_ADDRESS;

uint8_t deviceAddress() { 
	return i2c_address_current; 
};

uint8_t highAddressByte(word address)
{
	uint8_t BYTE_1 = address >> 8;
	return BYTE_1;
}

uint8_t lowAddressByte(word address)
{
	uint8_t BYTE_1 = address >> 8;
	uint8_t BYTE_2 = address - (BYTE_1 << 8);
	return BYTE_2;
}

/** \brief This function creates a Start condition (SDA low, then SCL low).
 * \return status of the operation
 * */
uint8_t i2c_send_start(void)
{
	Wire.beginTransmission(deviceAddress());
}


/** \brief This function creates a Stop condition (SCL high, then SDA high).
 * \return status of the operation
 * */
uint8_t i2c_send_stop(void)
{
 	uint8_t status = Wire.endTransmission();

 	delay(10);

 	if (status == 0) {
 		#if AES_LIBRARY_DEBUG
 			Serial.println("Stop sent succesfully");
 		#endif
 		return I2C_FUNCTION_RETCODE_SUCCESS;
 	} else if (status == 2 || status == 3) {
 		 #if AES_LIBRARY_DEBUG
 		 	Serial.println("NACK when sending stop");
 		 #endif
 		return I2C_FUNCTION_RETCODE_NACK;
 	} else if (status == 4) {
 		return I2C_FUNCTION_RETCODE_COMM_FAIL;
 	}

  	#if AES_LIBRARY_DEBUG
 		Serial.println("Data too long for buffer");
 	#endif

 	return I2C_FUNCTION_RETCODE_COMM_FAIL;
}

/** \brief This function sends bytes to an I2C device.
 * \param[in] count number of bytes to send
 * \param[in] data pointer to tx buffer
 * \return status of the operation
 */
uint8_t i2c_send_bytes(uint8_t count, uint8_t *data)
{
	#if AES_LIBRARY_DEBUG
		Serial.println("Sending bytes: ");
	#endif
	for (int i = 0; i < count; i++) {
		 #if AES_LIBRARY_DEBUG
			Serial.println(data[i], HEX);
		#endif
	}

	int sent_bytes = Wire.write(data, count);

	if (count > 0 && sent_bytes == count) {
		#if AES_LIBRARY_DEBUG
			Serial.println("Successfully sent all bytes.");
		#endif
		return I2C_FUNCTION_RETCODE_SUCCESS;
	}

	return I2C_FUNCTION_RETCODE_COMM_FAIL;
}

/** \brief This function receives bytes from an I2C device
 *         and sends a Stop.
 *
 * \param[in] count number of bytes to receive
 * \param[out] data pointer to rx buffer
 * \return status of the operation
 */
uint8_t i2c_receive_bytes(uint8_t count, uint8_t *data)
{
	uint8_t i;

	int available_bytes = Wire.requestFrom(deviceAddress(), count);
	delay(10);
	if (available_bytes != count) {
		return I2C_FUNCTION_RETCODE_COMM_FAIL;
	}

 	#if AES_LIBRARY_DEBUG
		Serial.println("Receiving bytes");
		Serial.println(count);
	#endif
	for (i = 0; i < count; i++) {
		while (!Wire.available()); // Wait for byte that is going to be read next
		byte c = Wire.read();
		#if AES_LIBRARY_DEBUG
			Serial.println("Read data:");
			Serial.println((int)c, BIN);
		#endif
		*data++ = c; // Store read value
	}

	return I2C_FUNCTION_RETCODE_SUCCESS;
}

/** \brief This function writes bytes to the device.
 * \param[in] count number of bytes to write
 * \param[in] word_address word address to write to
 * \param[in] data pointer to tx buffer
 * \return status of the operation
 */
uint8_t aes132p_write_memory_physical(uint8_t count, uint16_t word_address, uint8_t *data)
{
	#if AES_LIBRARY_DEBUG
		Serial.println("Calling write memory physical");
	#endif

	// Start sending data
	i2c_send_start();

	// Send the address
	uint8_t highAddress = highAddressByte(word_address);
	uint8_t lowAddress = lowAddressByte(word_address);

	i2c_send_bytes(1, &highAddress);
	i2c_send_bytes(1, &lowAddress);

	// Send the data
	i2c_send_bytes(count, data);

	// Wait
	delay(10);

	// End transmission
	return i2c_send_stop();
}


/** \brief This function reads bytes from the device.
 * \param[in] size number of bytes to write
 * \param[in] word_address word address to read from
 * \param[out] data pointer to rx buffer
 * \return status of the operation
 */
uint8_t aes132p_read_memory_physical(uint8_t size, uint16_t word_address, uint8_t *data)
{
	#if AES_LIBRARY_DEBUG
		Serial.println("Calling read memory physical");
	#endif

	// Start sending
	i2c_send_start();

	// Send the address
	uint8_t highAddress = highAddressByte(word_address);
	uint8_t lowAddress = lowAddressByte(word_address);

	i2c_send_bytes(1, &highAddress);
	i2c_send_bytes(1, &lowAddress);

	i2c_send_stop();

	// Actually listen for the bytes
	return i2c_receive_bytes(size, data);
}

/** \brief This function resynchronizes communication.
 * \return status of the operation
 */
uint8_t aes132p_resync_physical(void)
{
	#if AES_LIBRARY_DEBUG
		Serial.println("Resyncing");
	#endif

	// Try to re-synchronize without sending a Wake token
	// (step 1 of the re-synchronization process).
	uint8_t nine_clocks = 0xFF;
	Wire.beginTransmission(deviceAddress());
	i2c_send_bytes(1, &nine_clocks);
	uint8_t ret_code = Wire.endTransmission();

	if (ret_code == AES132_FUNCTION_RETCODE_SUCCESS) {
		return ret_code;
	} else {
		#if AES_LIBRARY_DEBUG
			Serial.println("Resync failed");
		#endif
		return AES132_FUNCTION_RETCODE_COMM_FAIL;
	}
}
