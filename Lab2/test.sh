#!bin/bash
concurrent_num=(10 20 30 40 50 60 70 80 90 100)
mkdir ans
for j in ${concurrent_num[*]} ; do
    echo ${j}
    ab -n 100 -c ${j} http://localhost:8080/api/check > ans/"${j}.txt"
done