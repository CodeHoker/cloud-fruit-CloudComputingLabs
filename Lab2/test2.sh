#!bin/basht
nums=(29 100 13 8 91 44)
array_name=(1 2 3 4 5 6 7 8 9 10)
ip="127.0.0.1"
a=8080
for i in ${array_name[*]} ; do
    port=`expr ${a} + ${i}`
   ./server --ip 127.0.0.1 --port 8080 --threads ${i}> ans.txt
done