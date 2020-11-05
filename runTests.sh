#!/bin/bash
inputDir=$1
outputDir=$2
maxThreads=$3

[ ! -d ${inputDir} ] && echo "A pasta de entrada ${inputDir} não existe." && exit

[ ! -d ${outputDir} ] && echo "A pasta de saída ${outputDir} não existe." && exit

if [[ ${maxThreads} =~ ^[\-0-9]+$ ]] && (( ${maxThreads} > 0)); then
    true
else
  echo "O valor de maxThreads não é um número ou é <= 0."
  echo "Repita a chamada ao script com um valor correcto de MaxThreads."
  exit
fi

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