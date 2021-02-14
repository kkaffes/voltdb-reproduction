load classes build/Procedures.jar;

DROP PROCEDURE InsertRecord IF EXISTS;
CREATE PROCEDURE PARTITION ON TABLE Reproduction COLUMN RecordID FROM CLASS procedures.InsertRecord;
