#!/bin/bash


for i in $(seq -f "%04g" 1 100)
do
	wget "https://github.com/KristofferKarlAxelEkstrand/AKWF-FREE/blob/master/AKWF/AKWF_0001/AKWF_${i}.wav?raw=true"
	mv "AKWF_${i}.wav?raw=true" "AKWF_${i}.wav"
done





