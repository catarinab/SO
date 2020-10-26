#!/bin/bash
inputDir=$1
outputDir=$2
maxThreads=$3

for inputFile in $(ls ${inputDir})
do
    for num in $(seq 1 ${maxThreads})
    do
        echo InputFile=${inputFile} NumThreads=${num}
        outputFile=$(echo "$inputFile" | cut -f 1 -d '.')
        outputFile=${outputFile}-${num}.txt
        output=$(./tecnicofs ${inputDir}/${inputFile} ${outputDir}/${outputFile} ${num} | tail -1)
        echo $output
    done
done