
logdir=$1
save_to=$2

for i in ${logdir}/compute_prob_valid.*; do
    trial=(${i//./ });
    trial=${trial[1]};

    grep -oP '(?<=accuracy for).+(?= per frame)' $i | while read -r match; do
        myarray=($match);
        echo "valid" "${myarray[0]}" "$trial" "${myarray[2]}" >> $save_to;
    done
done


for i in ${logdir}/compute_prob_train.*; do
    trial=(${i//./ });
    trial=${trial[1]};

    grep -oP '(?<=accuracy for).+(?= per frame)' $i | while read -r match; do
        myarray=($match);
        echo "train" "${myarray[0]}" "$trial" "${myarray[2]}" >> $save_to;
    done
done



