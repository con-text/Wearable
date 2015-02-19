crypto = require("crypto")
xor = require('bitwise-xor');

var encMode = "aes-128-cbc";
var ivBuf = new Buffer("00000000000000000000000000000000", 'hex');
var keyBuf = new Buffer("000102030405060708090a0b0c0d0e0f", 'hex');
var nonce = "000000000000000000000000";

/* 
ENCRYPTION 
http://www.atmel.com/Images/Atmel-8760-CryptoAuth-ATAES132-Datasheet.pdf
Page 113
*/

var plaintext = "68657979797979797979797979797979";

console.log("");
console.log("----ENCRYPTION----")
console.log("");
console.log("PLAINTEXT:");
printForArduino(plaintext);
console.log("");

// Construct B0 and calculate B'0
var B0 = new Buffer("79" + nonce + "010010", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp0 = cipher.update(B0,'hex','hex');
Bp0 += cipher.final('hex');

/*
Authenticate-only data:
manid = 0x00EE
encryptopcode = 0x06
mode = 0x00;
param1 = 0x0001
param2 = 0x0010
macflag = 0x00
0x0000000000
*/

// Construct B1 and calculate B'1
var B1 = xor(new Buffer(Bp0, 'hex'), new Buffer("000E00EE060000010010000000000000", 'hex')).toString('hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp1 = cipher.update(B1,'hex','hex');
Bp1 += cipher.final('hex');

// Construct B2 and calculate B'2
var B2 = xor(new Buffer(Bp1, 'hex'), new Buffer(plaintext, 'hex')).toString('hex');
B2 = new Buffer(B2, 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp2 = cipher.update(B2,'hex','hex');
Bp2 += cipher.final('hex');

// Construct A0 and calculate A'0
var A0 = new Buffer("01000000000000000000000000010000", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Ap0 = cipher.update(A0,'hex','hex');
Ap0 += cipher.final('hex');

// XOR the cleartext MAC (B'2) with A'0 
var Ap0xor = xor(new Buffer(Ap0, 'hex'), new Buffer(Bp2, 'hex')).toString('hex');
console.log("A'0 = " + Ap0xor.toUpperCase());

// Construct A1 and calculate A'1
var A1 = new Buffer("01000000000000000000000000010001", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Ap1 = cipher.update(A1,'hex','hex');
Ap1 += cipher.final('hex');

// XOR the cleartext data with A'1 
var Ap1xor = xor(new Buffer(Ap1, 'hex'), new Buffer(plaintext, 'hex')).toString('hex');
console.log("A'1 = " + Ap1xor.toUpperCase());

console.log("");

console.log("CIPHERTEXT:"); 
printForArduino(Ap1xor.toUpperCase());

console.log("");

console.log("MAC:"); 
printForArduino(Ap0xor.toUpperCase());

console.log("");

function printForArduino(string) {
	var newString = "";
	var string = string.toUpperCase();
	for (var i = 0; i < string.length; i = i + 2) {
		newString += "0x" + string[i] + string[i+1];
		if (i != string.length-2) newString += ", ";
	}

	console.log(newString);
}