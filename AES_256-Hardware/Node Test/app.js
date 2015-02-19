crypto = require("crypto")
xor = require('bitwise-xor');

var encMode = "aes-128-cbc";
var ivBuf = new Buffer("00000000000000000000000000000000", 'hex');
var keyBuf = new Buffer("000102030405060708090a0b0c0d0e0f", 'hex');
var nonce = "000000000000000000000000";

/* 
DECRYPTION 
http://www.atmel.com/Images/Atmel-8760-CryptoAuth-ATAES132-Datasheet.pdf
Page 114
*/

var ciphertext = "D5021639560E2829F2970F06187852DF";
var encInputMAC = "FEC17B9E6C91B9FC4A4F7D3A0390934C";

// Construct A0 and calculate A'0
var A0 = new Buffer("01000000000000000000000000010000", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Ap0 = cipher.update(A0,'hex','hex');
Ap0 += cipher.final('hex');

// Obtain cleartext MAC
var MACT = xor(new Buffer(Ap0, 'hex'), new Buffer(encInputMAC, 'hex')).toString('hex');
console.log("MACT = " + MACT.toUpperCase());

// Construct A1 and calculate A'1
var A1 = new Buffer("01000000000000000000000000010001", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Ap1 = cipher.update(A1,'hex','hex');
Ap1 += cipher.final('hex');

// Obtain cleartext message
var M = xor(new Buffer(Ap1, 'hex'), new Buffer(ciphertext, 'hex')).toString('hex');
console.log("M = " + M.toUpperCase());

// Construct B0 and calculate B'0
var B0 = new Buffer("79" + nonce + "010010", 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp0 = cipher.update(B0,'hex','hex');
Bp0 += cipher.final('hex');

// Construct B1 and calculate B'1
var B1 = xor(new Buffer(Bp0, 'hex'), new Buffer("000E00EE060000010010000000000000", 'hex')).toString('hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp1 = cipher.update(B1,'hex','hex');
Bp1 += cipher.final('hex');

// Construct B2 and calculate B'2
var B2 = xor(new Buffer(Bp1, 'hex'), new Buffer(M, 'hex')).toString('hex');
B2 = new Buffer(B2, 'hex');
var cipher = crypto.createCipheriv(encMode,keyBuf, ivBuf);
cipher.setAutoPadding(false);
var Bp2 = cipher.update(B2,'hex','hex');
Bp2 += cipher.final('hex');
console.log("Bp2 = " + Bp2.toUpperCase());

if (Bp2 === MACT) console.log("SUCCESS!");
else console.log("Failed.");

printForArduino("FEC17B9E6C91B9FC4A4F7D3A0390934C")

function printForArduino(string) {
	var newString = "";
	var string = string.toUpperCase();
	for (var i = 0; i < string.length; i = i + 2) {
		newString += "0x" + string[i] + string[i+1];
		if (i != string.length-2) newString += ", ";
	}

	console.log(newString);
}