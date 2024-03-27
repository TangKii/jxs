#!/bin/bash

CONF_JXS_DECODER=./jxs_decoder
CONF_JXS_ENCODER=./jxs_encoder
CONF_DIFFTEST=./difftest_ng
CONF_JXS_FOLDER=../../_wg1m92001
CONF_TMP_PGX="_tmp_xx"

echo "image,xs_config,img_config,enc,dec,psnr"

for JXS in $(find "${CONF_JXS_FOLDER}" -name "*.jxs"); do
  IMAGE_NAME="$(basename ${JXS})"
  echo -n "${IMAGE_NAME%.*}"
  # Decode XS configuration.
  CFG_LINE=$(${CONF_JXS_DECODER} -DD "${JXS}" 2>/dev/null | grep "Configuration: " | cut -d\" -f2)
  if [ $? -ne 0 ]; then
    echo ",ERR-parsing-XS-config"
    continue
  fi
  echo -n ",\"${CFG_LINE}\""
  # Decode image configuration.
  IMG_LINE=$(${CONF_JXS_DECODER} -v -D "${JXS}" 2>/dev/null | grep "Image: " | cut -d" " -f2-)
  if [ $? -ne 0 ]; then
    echo ",ERR-parsing-IMG-config"
    continue
  fi
  echo -n ",\"${IMG_LINE}\""
  # Encode using the parsed configuration.
  ${CONF_JXS_ENCODER} -c "${CFG_LINE}" "${JXS%.*}.pgx" "${CONF_TMP_PGX}.jxs" >/dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo ",ERR-encoding"
    continue
  fi
  echo -n ",OK"
  # Decode the just encoded JXS.
  ${CONF_JXS_DECODER} "${CONF_TMP_PGX}.jxs" "${CONF_TMP_PGX}.pgx" >/dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo ",ERR-decoding"
    continue
  fi
  echo -n ",OK"
  # PSNR the data (it will not match exactly, as encoder is different)
  PSNR=$(${CONF_DIFFTEST} --psnr "${CONF_TMP_PGX}.pgx" "${JXS%.*}.pgx" | cut -d":" -f2 | xargs)
  echo -n ",${PSNR}"
  # Cleanup.
  rm "${CONF_TMP_PGX}.jxs" >/dev/null 2>&1
  rm "${CONF_TMP_PGX}.pgx" >/dev/null 2>&1
  for I in 0 1 2 3; do
    rm "${CONF_TMP_PGX}_${I}.raw" >/dev/null 2>&1
    rm "${CONF_TMP_PGX}_${I}.h" >/dev/null 2>&1
  done
  echo
done
