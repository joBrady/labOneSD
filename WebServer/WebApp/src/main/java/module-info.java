module com.example.webapp {
    requires javafx.controls;
    requires javafx.fxml;
    requires twilio;
    requires java.sql;

            
        requires org.controlsfx.controls;
                        requires org.kordamp.bootstrapfx.core;
            
    opens com.example.webapp to javafx.fxml;
    exports com.example.webapp;
}