Natalie Valett
nvalett
CSE13S Assignment 7: The Great Firewall of Santa Cruz

This program is designed to protect the great city of Santa Cruz from the greatest impending terror facing our great nation: thoughtcrime. In an attempt to reenforce Santa Cruz's new speech regulations and rectify the language of our nation, every citizen's speech will be under the close scrutiny of The Great Firewall, a language filter which will inform citizens of their transgressions, as well as how to redeem themselves with proper speech in the future. The Great Firewall processes each word spoken in Santa Cruz through a bloom filter, which if triggered will further scrutinize the word with the use of the trusty hash table, which reports if in fact the word in question constitutes use of oldspeak, or worse... thoughtcrime. Upon evaluation, the transgressor will recieve a message detailing their misspeaks and their according punishments.

TO RUN:
After building the project by entering 'make' in the console, this should produce a new executable in the directory called banhammer. To run, enter "./banhammer [options]" in the terminal, where [options include:

	-h: Prints out a help message for program usage
	-t specifies that the hash table will havesizeentries (the default will be 10000)
        -f specifies that the Bloom filter will havesizeentries (the default will be 220)
        -m will enable themove-to-front rule. By default, the move-to-front rule is disabled.
        -s will enable the printing of statistics to stdout.  The statistics to calculate are the total number ofseeks, the average seek length, the hash table load, and the Bloom filter load
