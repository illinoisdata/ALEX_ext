set -e

DATASET_NAMES=(
  "books 800 uint64"
  "fb 200 uint64"
  "osm_cellids 800 uint64"
  "wiki_ts 200 uint64"
  "gmm_k100 800 uint64"
)

DB_PATH=$1
KEYSET_PATH=$2
OUT_PATH=$3
RELOAD_FILE=$4

mkdir -p ${OUT_PATH}

echo "Start ALEX Benchmark"

for ((i = 0; i < ${#DATASET_NAMES[@]}; i++)) do
 for ((j = 0; j < 40; j++)) do
    read -a dataset_blob <<< ${DATASET_NAMES[$i]}	 
    dataset_name=${dataset_blob[0]}_${dataset_blob[1]}M_${dataset_blob[2]}     
    
    echo ">>> ${dataset_name} ${j}"
    bash ${RELOAD_FILE}
    ./build/kv_benchmark \
        --key_path=${KEYSET_PATH}/${dataset_name}_ks_${j} \
        --target_db_path=${DB_PATH}/${dataset_name} \
        --out_path=${OUT_PATH}/${dataset_name}_out.txt
 done
done
