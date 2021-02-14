#!/bin/bash
set -ex

SCRIPT_DIR=$(dirname $(readlink -f $0))
# Enter the root dir of the repo.
cd ${SCRIPT_DIR}/../

# Create obj directory
mkdir -p obj/sql

javac -classpath "$VOLT_HOME/voltdb/*" -d obj/sql sql/*.java
jar cvf build/Procedures.jar -C obj/sql .
${VOLTDB_BIN}/sqlcmd < sql/load_procedure.sql
