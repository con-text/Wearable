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

/** \brief I2C address used at AES132 library startup. */
#define AES132_I2C_DEFAULT_ADDRESS   ((uint8_t) 0x50)


/** \brief These enumerations are flags for I2C read or write addressing. */
enum aes132_i2c_read_write_flag {
	I2C_WRITE = (uint8_t) 0x00,	//!< write command id
	I2C_READ  = (uint8_t) 0x01   //! read command id
};

//! I2C address currently in use.
static uint8_t i2c_address_current = AES132_I2C_DEFAULT_ADDRESS;

/** \brief This function initializes and enables the I2C peripheral.
 * */
void i2c_enable(void)
{
	Serial.println("I2C_Enable");
	chip_wakeup();
}


/** \brief This function disables the I2C peripheral. */
//TODO: Put it into sleep mode
void i2c_disable(void)
{
	Serial.println("I2C_Disable");
}

uint8_t deviceAddress() { 
	return i2c_address_current; 
};

void chip_wakeup()
{
	Serial.println("chip_wakeup()");
	// This was the only way short of manually adjusting the SDA pin to wake up the device
	Wire.beginTransmission(deviceAddress());
  	int status = Wire.endTransmission();
  	Serial.println("Sent begin and end - return = ");
  	Serial.print(status);
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

 	Serial.print("Stop status, returns:");
 	Serial.println(status);

 	if (status == 0) {
 		Serial.println("Stop sent succesfully");
 		return I2C_FUNCTION_RETCODE_SUCCESS;
 	} else if (status == 2 || status == 3) {
 		 Serial.println("NACK when sending stop");
 		return I2C_FUNCTION_RETCODE_NACK;
 	} else if (status == 4) {
 		return I2C_FUNCTION_RETCODE_COMM_FAIL;
 	}

 	Serial.println("Data too long for buffer");

 	return I2C_FUNCTION_RETCODE_COMM_FAIL;
}


/** \brief This function sends bytes to an I2C device.
 * \param[in] count number of bytes to send
 * \param[in] data pointer to tx buffer
 * \return status of the operation
 */
uint8_t i2c_send_bytes(uint8_t count, uint8_t *data)
{
	int sent_bytes = Wire.write(data, count);

	if (count > 0 && sent_bytes == count) {
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
	if (available_bytes != count) {
		return I2C_FUNCTION_RETCODE_COMM_FAIL;
	}

	for (i = 0; i < count; i++) {
		while (!Wire.available()); // Wait for byte that is going to be read next
		*data++ = Wire.read(); // Store read value
	}

	return i2c_send_stop();
}

 /** \brief This function creates a Start condition and sends the I2C address.
 * \param[in] read I2C_READ for reading, I2C_WRITE for writing
 * \return status of the operation
 */
uint8_t aes132p_send_slave_address(uint8_t read)
{
	uint8_t sla = i2c_address_current | read;
	uint8_t aes132_lib_return;
	Wire.beginTransmission(deviceAddress());

	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS)
		return aes132_lib_return;

	aes132_lib_return = i2c_send_bytes(1, &sla);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
		i2c_send_stop();
		Serial.println("Sending initial address failed");
	}

	return aes132_lib_return;
}

/** \brief This function initializes and enables the I2C hardware peripheral. */
void aes132p_enable_interface(void)
{
	i2c_enable();
}


/** \brief This function disables the I2C hardware peripheral. */
void aes132p_disable_interface(void)
{
	i2c_disable();
}


/** \brief This function selects a I2C AES132 device.
 *
 * @param[in] device_id I2C address
 * @return always success
 */
 //TODO: Look at if this is necessary
uint8_t aes132p_select_device(uint8_t device_id)
{
	//i2c_address_current = device_id & ~1;
	return AES132_FUNCTION_RETCODE_SUCCESS;
}


/** \brief This function writes bytes to the device.
 * \param[in] count number of bytes to write
 * \param[in] word_address word address to write to
 * \param[in] data pointer to tx buffer
 * \return status of the operation
 */
uint8_t aes132p_write_memory_physical(uint8_t count, uint16_t word_address, uint8_t *data)
{
	Serial.println("Calling write memory physical");

	// In both, big-endian and little-endian systems, we send MSB first.
	const uint8_t word_address_buffer[] = {(uint8_t) (word_address >> 8), (uint8_t) (word_address & 0xFF)};
	uint8_t aes132_lib_return = aes132p_send_slave_address(I2C_WRITE);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS)
		// There is no need to create a Stop condition, since function
		// aes132p_send_slave_address does that already in case of error.
		return aes132_lib_return;

	aes132_lib_return = i2c_send_bytes(sizeof(word_address), (uint8_t *) word_address_buffer);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
		// Don't override the return code from i2c_send_bytes in case of error.
		i2c_send_stop();
		Serial.println("Writing memory failed");
		return aes132_lib_return;
	}

	if (count > 0)
		// A count of zero covers the case when resetting the I/O buffer address.
		// This case does only require a write access to the device,
		// but data don't have to be actually written.
		aes132_lib_return = i2c_send_bytes(count, data);

	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
		// Don't override the return code from i2c_send_bytes in case of error.
		i2c_send_stop();
		return aes132_lib_return;
	}

	// success
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
	Serial.println("Calling read memory physical");

	// Random read:
	// Start, I2C address with write bit, word address,
	// Start, I2C address with read bit

	// In both, big-endian and little-endian systems, we send MSB first.
	const uint8_t word_address_buffer[] = {(uint8_t) (word_address >> 8), (uint8_t) (word_address & 0xFF)};

	uint8_t aes132_lib_return = aes132p_send_slave_address(I2C_WRITE);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS)
		return aes132_lib_return;

	//Send the word address
	aes132_lib_return = i2c_send_bytes(sizeof(word_address), (uint8_t *)word_address_buffer);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
		i2c_send_stop();
		Serial.println("Read memory failed");
		return aes132_lib_return;
	}

	// Start reading over I2C
	aes132_lib_return = aes132p_send_slave_address(I2C_READ);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS)
		return aes132_lib_return;

	// Actually listen for the bytes
	return i2c_receive_bytes(size, data);
}

/** \brief This function resynchronizes communication.
 * \return status of the operation
 */
uint8_t aes132p_resync_physical(void)
{
	Serial.println("Resyncing");

	// Try to re-synchronize without sending a Wake token
	// (step 1 of the re-synchronization process).
	uint8_t nine_clocks = 0xFF;
	Wire.beginTransmission(deviceAddress());
	i2c_send_bytes(1, &nine_clocks);
	uint8_t ret_code = Wire.endTransmission();

	if (ret_code == AES132_FUNCTION_RETCODE_SUCCESS) {
		return ret_code;
	} else {
		Serial.println("Resync failed");
		return AES132_FUNCTION_RETCODE_COMM_FAIL;
	}
}
