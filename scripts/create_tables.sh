#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# Create tables.
${VOLTDB_BIN}/sqlcmd < sql/create_table.sql
