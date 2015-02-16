#include <Wire.h>
#include <aes132.h>
//#include <aes132_i2c.h>
#include <aes132_commands.h>

byte val = 0;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  while (!Serial);

  Wire.begin();
      
  uint16_t address = 0xF040;
  uint8_t rxBuffer[AES132_RESPONSE_SIZE_MIN + 1] = {0};
  aes132m_block_read(address, 1, rxBuffer);
  delay(100);
 // uint16_t address = 0xF010;
  //aes132c_wri(address, 2, rx_buffer);
  
  for (int i = 0; i < sizeof(rxBuffer); i++) {
    Serial.println(rxBuffer[i], HEX);
  } 
 
   //getInfo();
   //calculateTemperature();
   
   //testWriteRead();
}

void loop() {
  // put your main code here, to run repeatedly:

}

byte calculateTemperature()
{
  uint16_t response[1] = {0};
  
  uint8_t ret_code = aes132m_temp_sense(response);
  Serial.println("Response bytes:");
  for (int i = 0; i < sizeof(response); i++) {
    Serial.println(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}

byte getInfo()
{
  uint8_t response[2] = {0,0};
  
  uint8_t ret_code = aes132m_info(6, response);
  Serial.println("Response bytes:");
  for (int i = 0; i < sizeof(response); i++) {
    Serial.print(response[i], HEX);
  }
  Serial.println();

  return ret_code;  
}

int testWriteRead(void)
{
	uint16_t word_address = 0x0000;
	uint8_t rx_byte_count = 4;
	uint8_t tx_buffer_write[] = {0x55, 0xAA, 0xBC, 0xDE};
	uint8_t tx_buffer_command[AES132_COMMAND_SIZE_MIN];
	uint8_t rx_buffer[AES132_RESPONSE_SIZE_MIN + 4]; // 4: number of bytes to read using the BlockRead command
	uint8_t aes132_lib_return;

	aes132p_enable_interface();

	// -------------------- Write memory. -----------------------------------
   // Don't put this in an infinite loop. Otherwise the non-volatile memory will wear out.
	aes132_lib_return = aes132c_write_memory(sizeof(tx_buffer_write), word_address, tx_buffer_write);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
	   aes132p_disable_interface();
		return aes132_lib_return;
	}

        Serial.println("Written memory.");

	// -------------------- Read memory. -----------------------------------
	aes132_lib_return = aes132c_read_memory(rx_byte_count, word_address, rx_buffer);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
	   aes132p_disable_interface();
		return aes132_lib_return;
	}

        Serial.println("Read memory.");

	// -------------------- Compare written with read data. -----------------------------------
	aes132_lib_return =  memcmp(tx_buffer_write, rx_buffer, sizeof(tx_buffer_write));
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
	   aes132p_disable_interface();
		return aes132_lib_return;
	}

        Serial.println("Compared memory.");

	// ------- Send a BlockRead command and receive its response. -----------------------------
	aes132_lib_return = aes132m_execute(AES132_OPCODE_BLOCK_READ, 0, word_address, rx_byte_count,
	                                    0, NULL, 0, NULL, 0, NULL, 0, NULL, tx_buffer_command, rx_buffer);
	if (aes132_lib_return != AES132_FUNCTION_RETCODE_SUCCESS) {
	   aes132p_disable_interface();
		return aes132_lib_return;
	}

        Serial.println("BlockRead memory.");

	// -------------------- Compare written with read data. -----------------------------------
	aes132_lib_return =  memcmp(tx_buffer_write, &rx_buffer[AES132_RESPONSE_INDEX_DATA], sizeof(tx_buffer_write));

        Serial.println("Compared memory.");
        Serial.println(aes132_lib_return);

	aes132p_disable_interface();

	return aes132_lib_return;
}
