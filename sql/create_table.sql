CREATE TABLE Reproduction (
    RecordID INTEGER NOT NULL,
    Data VARCHAR(1048576) NOT NULL
);
PARTITION TABLE Reproduction ON COLUMN RecordID;
