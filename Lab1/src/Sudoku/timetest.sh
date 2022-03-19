for ((i=1; i<=10; i++))
do
  cat test_group| ./sudoku_solve $i
done