
cleanup(){
	rm -r test_dir
}

QUEUE="queue.o queue.h"
TEST_FILES="test_files/multi-consumer_thread_order.c test_files/Makefile"

mkdir test_dir

cp $QUEUE test_dir
cp $TEST_FILES test_dir

make -C test_dir

./test_dir/queue-test 100

if [ $? != 0 ]
then
	echo "bad exit"
	cleanup
	exit 1
fi

cleanup
