set -e

DATASET_NAMES=(
  "books 800 800000000 uint64"
  "fb 200 200000000 uint64"
  "osm_cellids 800 800000000 uint64"
  "wiki_ts 200 200000000 uint64"
  "gmm_k100 800 800000000 uint64"
)

BLOB_PATH=$1
DATA_PATH=$2

mkdir -p ${DATA_PATH}

echo "Start Building ALEX"

for ((i = 0; i < ${#DATASET_NAMES[@]}; i++)) do
    read -a dataset_blob <<< ${DATASET_NAMES[$i]}

    num_keys=${dataset_blob[2]}
    dataset_name=${dataset_blob[0]}_${dataset_blob[1]}M_${dataset_blob[3]}

    echo ">>> ${dataset_name}"
    ./build/kv_build \
        --keys_file=${BLOB_PATH}/${dataset_name} \
        --keys_file_type=sosd \
        --total_num_keys=${num_keys} \
        --db_path=${DATA_PATH}/${dataset_name}
done
