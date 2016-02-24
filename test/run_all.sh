#!/bin/bash

stats=`grep -x "#define ENABLE_STATS" ../include/config.h`
if [ -n "$stats" ]; then
	echo "Disable Statistics"
	sed -i -e 's/\#define ENABLE_STATS/\/\/\#define ENABLE_STATS/' ../include/config.h
fi
mkdir -p logs
#cat ../include/config.h
echo "******************************************************************"
echo "Build Library."
make -C .. lib
echo "******************************************************************"
echo "Build Tests."
make all
echo "******************************************************************"
echo "test1"
mkdir -p logs/test1
rm -fr logs/test1/*.log
for i in {1..1000}
do
./test1/test1.bin 1024 1 > ./logs/test1/test1_1_$i.log
./test1/test1.bin 1024 2 > ./logs/test1/test1_2_$i.log
./test1/test1.bin 1024 4 > ./logs/test1/test1_4_$i.log
./test1/test1.bin 1024 8 > ./logs/test1/test1_8_$i.log
done
echo "******************************************************************"
echo "test2"
mkdir -p logs/test2
rm -fr logs/test2/*.log
for i in {1..1000}
do
./test2/test2.bin 1024 1 > ./logs/test2/test2_1_$i.log
./test2/test2.bin 1024 2 > ./logs/test2/test2_2_$i.log
./test2/test2.bin 1024 4 > ./logs/test2/test2_4_$i.log
./test2/test2.bin 1024 8 > ./logs/test2/test2_8_$i.log
done
echo "******************************************************************"
echo "test3"
mkdir -p logs/test3
rm -fr logs/test3/*.log
for i in {1..1000}
do
./test3/test3.bin 1024 1 > ./logs/test3/test3_1_$i.log
./test3/test3.bin 1024 2 > ./logs/test3/test3_2_$i.log
./test3/test3.bin 1024 4 > ./logs/test3/test3_4_$i.log
./test3/test3.bin 1024 8 > ./logs/test3/test3_8_$i.log
done
echo "******************************************************************"
echo "test4"
mkdir -p logs/test4
rm -fr logs/test4/*.log
for i in {0..1000}
do
./test4/test4.bin 1024 1 > ./logs/test4/test4_1_$i.log
./test4/test4.bin 1024 2 > ./logs/test4/test4_2_$i.log
./test4/test4.bin 1024 4 > ./logs/test4/test4_4_$i.log
./test4/test4.bin 1024 8 > ./logs/test4/test4_8_$i.log
done
#echo "******************************************************************"
#echo "fibonacci"
#./fibonacci/fibonacci.bin 39 1
#./fibonacci/fibonacci.bin 39 2
#./fibonacci/fibonacci.bin 39 4
#./fibonacci/fibonacci.bin 39 8
echo "******************************************************************"
echo "Tests completed."
echo "******************************************************************"
echo "Clear Tests and Library."
make -C .. clear
echo "******************************************************************"

stats=`grep -x "//#define ENABLE_STATS" ../include/config.h`
if [ -n "$stats" ]; then
	echo "Enable Statistics"
	sed -i -e 's/\/\/\#define ENABLE_STATS/\#define ENABLE_STATS/' ../include/config.h
fi
#cat ../include/config.h
