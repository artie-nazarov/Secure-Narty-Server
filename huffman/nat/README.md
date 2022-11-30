Natalie Valett
nvalett
CSE13S Assignment 6: Huffman Coding


This lab’s objective is to implement the Huffman Code algorithm for data compression and decompression. The encoder takes in the given input, computes a histogram representing the frequency of each character present in the file. From the frequency histogram it constructs a binary tree whose leaves represent each unique character, and whose distance from the node is proportional to that characters frequency. The program utilizes this tree to calculate shortened codes for each character. Once this is complete the encoder is able to transmit the same message in less bits, by transmitting over the tree and the data of the origional file encrypted with the character codes. After reconstructing the binary tree the decoder is able to decode the encrypted message by itteratively traversing the tree until a leaf (symbol) is met. In this way the decoder is able to reconstruct the origional message, and will print it to output.

TO RUN:
After building the project by entering 'make' in the console, there should be TWO new executables in the directory called encode and decode. 

ENCODE:
To run the encoder enter "./encode [options]" in the terminal, where [options] include:
	-h: Prints out a help message for program usage
	-v: verbose printing, prints encoding data/stats
	-i infile: File to be encrypted. If not specified, defaults to stdin.
	-o outfile: File where encrypted output will be printed. If not specified, defaults to stdout.

DECODE:
To run the decoder enter "./decode [options]" in the terminal, where [options] include:
	-h: Prints out a help message for program usage
	-v: Enables verbose printing. If enabled, the program will print out decoding stats/data
	-i infile: File to be encrypted. If not specified, defaults to stdin.
	-o outfile: File where encrypted output will be printed. If not specified, defaults to stdout.
