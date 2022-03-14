#!/bin/bash

echo "Parallelizing over output channels, square panels"
export OMP_NUM_THREADS=6;
echo $OMP_NUM_THREADS;

#32 48 64 96 128 144 192 256 384 512 768 1024 ;

for i in 12 36 96 126 252 504;
do

    for j in 64;
    do
	
		 for k in 64;

		 do
		     echo -n  ${i},  ${k},
		     $1  ${j} ${k} ${i} ${i}
		 done 
    done
done
