#!/bin/bash

CONF_JXS_DECODER=../_build_wsl/bin/jxs_decoder
CONF_JXS_FOLDER=../../_cache/jpegxs_ref_iso21122-4_ed2
CONF_DIFFTEST=./difftest_ng
CONF_TMP_PGX="_tmp_xx"

echo "image,decoded,raw_size,checksum"

for JXS in $(find "${CONF_JXS_FOLDER}" -name "*.jxs"); do
  IMAGE_NAME="$(basename ${JXS})"
  echo -n "${IMAGE_NAME%.*}"
  # Decode.
  ${CONF_JXS_DECODER} "${JXS}" "${CONF_TMP_PGX}.pgx" >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo -n ",OK"
    # Checksum of PGX RAW binary data.
    RAW_BYTES=$(cat "${CONF_TMP_PGX}.pgx" | xargs du -sb | nawk '{sum+=$1} END {print sum}')
    echo -n ",${RAW_BYTES}"
    CHECKSUM_REC=$(cat ${CONF_TMP_PGX}.pgx | xargs cat | sha256sum - | cut -f1 -d" ")
    CHECKSUM_SRC=$(cat ${JXS%.*}.pgx | sed 's;^;'${CONF_JXS_FOLDER}'/;' | xargs cat | sha256sum - | cut -f1 -d" ")
    if [[ "${CHECKSUM_REC}" == "${CHECKSUM_SRC}" ]]; then
      echo -n ",MATCH"
    else
      # PSNR the data (as the match was not exact)
      PSNR=$(${CONF_DIFFTEST} --psnr "${CONF_TMP_PGX}.pgx" "${JXS%.*}.pgx" | cut -d":" -f2 | xargs)
      echo -n ",${PSNR}"
    fi
  else
    echo -n ",ERR,NA,NA"
  fi
  # Cleanup.
  rm "${CONF_TMP_PGX}.pgx" >/dev/null 2>&1
  for I in 0 1 2 3; do
    rm "${CONF_TMP_PGX}_${I}.raw" >/dev/null 2>&1
    rm "${CONF_TMP_PGX}_${I}.h" >/dev/null 2>&1
  done
  echo
done
