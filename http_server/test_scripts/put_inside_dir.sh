#!/usr/bin/env bash

port=$(bash test_files/get_port.sh)

# Start up server.
./httpserver $port > /dev/null &
pid=$!

# Wait until we can connect.
while ! nc -zv localhost $port; do
    sleep 0.01
done

for i in {1..10}; do
    # Test input file.
    dir="oogaboogadir"
    rm -rf $dir
    mkdir $dir
    file="test_files/wonderful.txt"
    infile="$dir/temp.txt"
    outfile="outtemp.txt"

    # Create the input file to overwrite.
    echo "ooga booga ya?" > $infile

    # Expected status code.
    expected=400

    # The only thing that is should be printed is the status code.
    actual=$(curl -s -w "%{http_code}" -o $outfile localhost:$port/$infile -T $file)

    # Check the status code.
    if [[ $actual -ne $expected ]]; then
        # Make sure the server is dead.
        kill -9 $pid
        wait $pid
        rm -f $infile $outfile
        rm -rf $dir
        exit 1
    fi

    # Clean up.
    rm -f $infile $outfile
    rm -rf $dir
done

# Clean up.
rm -f $infile $outfile
rm -rf $dir

exit 0