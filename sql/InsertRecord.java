package procedures;

import org.voltdb.*;

public class InsertRecord extends VoltProcedure {
    final long SUCCESS = 0;

    public final SQLStmt insertRecord = new SQLStmt (
        "INSERT INTO Reproduction VALUES(?,?);"
    );

    public long run(int recordID, String data) throws VoltAbortException {
        voltQueueSQL(insertRecord, recordID, data);
        voltExecuteSQL();
        return SUCCESS;
    }
}
