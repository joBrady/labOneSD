package com.example.webapp; // Replace with your actual package name

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.NumberAxis;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

import com.twilio.Twilio;
import com.twilio.rest.api.v2010.account.Message;
import com.twilio.type.PhoneNumber;

public class HelloApplication extends Application {

    public static final String ACCOUNT_SID = "AC7a5ed473f654f1e15339367fa8c432b3";
    public static final String AUTH_TOKEN = "04aa0ad34bc57e95b762624c33501d6e";
    private static final int SERVER_PORT = 3000; // Port for receiving data from ESP32
    private static final String ESP32_IP = "192.168.139.153"; // Replace with your ESP32 IP address
    private static final int COMMAND_PORT = 3001; // Port for sending commands to ESP32

    private XYChart.Series<Number, Number> sensor1Series = new XYChart.Series<>();
    private XYChart.Series<Number, Number> sensor2Series = new XYChart.Series<>();
    private int time = 0; // Time variable to serve as x-axis value (seconds ago)

    // Labels to display the real-time temperature values
    private Label sensor1Label = new Label("Sensor 1: N/A");
    private Label sensor2Label = new Label("Sensor 2: N/A");

    // TextFields to set minimum and maximum temperature bounds
    private TextField minTempField = new TextField();
    private TextField maxTempField = new TextField();
    private TextField phoneNumberField = new TextField();

    // Toggle buttons to control sensor display on ESP32
    private Button toggleSensor1Button = new Button("Toggle Sensor 1 Display");
    private Button toggleSensor2Button = new Button("Toggle Sensor 2 Display");

    // Celsius/Fahrenheit toggle button and unit flag
    private Button toggleUnitButton = new Button("Switch to Fahrenheit");
    private boolean isCelsius = true; // Track if displaying Celsius or Fahrenheit

    // Define the yAxis globally so we can change its label dynamically
    private NumberAxis yAxis = new NumberAxis(10, 50, 10); // Default range for Celsius

    public static void main(String[] args) {
        Twilio.init(ACCOUNT_SID, AUTH_TOKEN);
        launch(args);
    }

    @Override
    public void start(Stage primaryStage) {
        // Set up the labels for displaying real-time data
        sensor1Label.setStyle("-fx-font-size: 20;");
        sensor2Label.setStyle("-fx-font-size: 20;");

        // Set up buttons for controlling sensor display on ESP32
        toggleSensor1Button.setOnAction(event -> sendCommand("toggleSensor1"));
        toggleSensor2Button.setOnAction(event -> sendCommand("toggleSensor2"));

        // Set up the toggle button for Celsius/Fahrenheit
        toggleUnitButton.setOnAction(event -> toggleTemperatureUnit());

        // Set up min/max temperature fields
        Label minTempLabel = new Label("Min Temp:");
        minTempField.setPromptText("Enter min temp");
        minTempField.setMaxWidth(100);

        Label maxTempLabel = new Label("Max Temp:");
        maxTempField.setPromptText("Enter max temp");
        maxTempField.setMaxWidth(100);

        // Set up phone number field for user input
        Label phoneNumberLabel = new Label("Recipient Phone:");
        phoneNumberField.setPromptText("+1234567890");
        phoneNumberField.setMaxWidth(200);

        HBox tempBoundsBox = new HBox(10, minTempLabel, minTempField, maxTempLabel, maxTempField);
        tempBoundsBox.setAlignment(Pos.CENTER_LEFT);

        HBox phoneNumberBox = new HBox(10, phoneNumberLabel, phoneNumberField);
        phoneNumberBox.setAlignment(Pos.CENTER_LEFT);

        // Create a VBox to hold the labels, buttons, and temp fields
        VBox controlBox = new VBox(20, sensor1Label, toggleSensor1Button, sensor2Label, toggleSensor2Button, toggleUnitButton, tempBoundsBox, phoneNumberBox);
        controlBox.setAlignment(Pos.CENTER_LEFT);
        controlBox.setPadding(new Insets(20));

        // Create the x-axis and y-axis for the sensor graph
        NumberAxis xAxis = new NumberAxis(-300, 0, 50); // Rightmost point is 0, leftmost is -300
        xAxis.setLabel("Time (seconds ago)");
        xAxis.setAutoRanging(false); // Fixed range for the x-axis

        yAxis.setLabel("Temperature (°C)"); // Default label is Celsius
        yAxis.setAutoRanging(false); // Fixed range for the y-axis

        // Create the line chart for the sensor data
        LineChart<Number, Number> sensorChart = new LineChart<>(xAxis, yAxis);
        sensorChart.setTitle("Real-Time Temperature Data");
        sensor1Series.setName("Sensor 1");
        sensor2Series.setName("Sensor 2");
        sensorChart.getData().add(sensor1Series);
        sensorChart.getData().add(sensor2Series);

        // Disable symbols (dots) for the entire line chart to only show lines
        sensorChart.setCreateSymbols(false);

        // Create an HBox to place the chart and control box side by side
        HBox chartBox = new HBox(20, controlBox, sensorChart);
        chartBox.setAlignment(Pos.CENTER);

        // Set up the scene and stage
        Scene scene = new Scene(chartBox, 1400, 600); // Adjust width for charts and controls
        primaryStage.setScene(scene);
        primaryStage.setTitle("ESP32 Temperature Data Visualizer with Control");
        primaryStage.show();

        // Start the server socket in a new thread to receive data from ESP32
        new Thread(this::startDataServer).start();
    }

    /**
     * Starts the server to receive data from the ESP32.
     */
    private void startDataServer() {
        try (ServerSocket serverSocket = new ServerSocket(SERVER_PORT)) {
            System.out.println("Data server started. Waiting for ESP32 data...");

            while (true) {
                // Accept incoming connection from ESP32
                try (Socket clientSocket = serverSocket.accept();
                     BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()))) {

                    // Read data from ESP32
                    String data = in.readLine();
                    System.out.println("Received: " + data);

                    // Check for "no_data" message from the ESP32
                    if (data != null && data.equals("no_data")) {
                        Platform.runLater(this::showNoDataAvailable);
                        continue;
                    }

                    // Parse JSON-like data (example format: {"sensor1":25.6,"sensor2":26.1})
                    if (data != null && data.contains("sensor1") && data.contains("sensor2")) {
                        try {
                            double sensor1Value = Double.parseDouble(data.split(",")[0].split(":")[1]);
                            double sensor2Value = Double.parseDouble(data.split(",")[1].split(":")[1].replace("}", ""));

                            // Check if temperature is out of bounds and send alert
                            checkTemperatureAndSendAlert(sensor1Value, sensor2Value);

                            // Update the chart and labels on the JavaFX Application Thread
                            Platform.runLater(() -> updateChartsAndLabels(sensor1Value, sensor2Value));
                        } catch (NumberFormatException e) {
                            System.out.println("Failed to parse data: " + e.getMessage());
                        }
                    }
                } catch (Exception e) {
                    System.out.println("Error handling client connection: " + e.getMessage());
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Displays "No Data Available" on the labels and removes points from the chart.
     */
    private void showNoDataAvailable() {
        sensor1Label.setText("Sensor 1: No Data Available");
        sensor2Label.setText("Sensor 2: No Data Available");

        // Add missing data indicators on the graph (e.g., null points)
        for (XYChart.Series<Number, Number> series : new XYChart.Series[]{sensor1Series, sensor2Series}) {
            series.getData().add(new XYChart.Data<>(0, null));
        }
    }

    /**
     * Checks if the temperature is outside the bounds set by the user and sends an alert if it is.
     */
    private void checkTemperatureAndSendAlert(double sensor1Value, double sensor2Value) {
        try {
            double minTemp = Double.parseDouble(minTempField.getText());
            double maxTemp = Double.parseDouble(maxTempField.getText());

            // Check if either sensor's value is outside the specified bounds
            if (sensor1Value < minTemp || sensor1Value > maxTemp || sensor2Value < minTemp || sensor2Value > maxTemp) {
                sendTwilioAlert(sensor1Value, sensor2Value, minTemp, maxTemp);
            }
        } catch (NumberFormatException e) {
            System.out.println("Invalid min/max temperature values. Please enter valid numbers.");
        }
    }

    /**
     * Sends a Twilio alert message if the temperature is out of bounds.
     */
    private void sendTwilioAlert(double sensor1Value, double sensor2Value, double minTemp, double maxTemp) {
        try {
            // Get the phone number from the input field
            String recipientPhoneNumber = phoneNumberField.getText();

            if (recipientPhoneNumber == null || recipientPhoneNumber.isEmpty()) {
                System.out.println("Phone number is empty. Cannot send alert.");
                return;
            }

            String twilioPhoneNumber = "+18887927957"; // Replace with your Twilio phone number

            String messageBody = String.format(
                    "Alert! Sensor 1: %.2f, Sensor 2: %.2f, which is out of bounds (%.2f - %.2f).",
                    sensor1Value, sensor2Value, minTemp, maxTemp
            );

            Message message = Message.creator(
                    new PhoneNumber(recipientPhoneNumber),
                    new PhoneNumber(twilioPhoneNumber),
                    messageBody
            ).create();

            System.out.println("Sent alert: " + message.getSid());
        } catch (Exception e) {
            System.out.println("Failed to send Twilio alert: " + e.getMessage());
        }
    }

    // Other methods like toggleTemperatureUnit(), updateChartsAndLabels(), etc. remain unchanged.
    /**
     * Toggles the temperature unit between Celsius and Fahrenheit.
     */
    private void toggleTemperatureUnit() {
        isCelsius = !isCelsius; // Toggle the unit flag

        if (isCelsius) {
            toggleUnitButton.setText("Switch to Fahrenheit");
            yAxis.setLabel("Temperature (°C)");
            yAxis.setLowerBound(10);  // Set the lower bound to 10°C
            yAxis.setUpperBound(50);  // Set the upper bound to 50°C
        } else {
            toggleUnitButton.setText("Switch to Celsius");
            yAxis.setLabel("Temperature (°F)");
            yAxis.setLowerBound(celsiusToFahrenheit(10));  // Convert lower bound to Fahrenheit
            yAxis.setUpperBound(celsiusToFahrenheit(50));  // Convert upper bound to Fahrenheit
        }

        // Convert all existing data points in both series
        convertExistingDataPoints(sensor1Series);
        convertExistingDataPoints(sensor2Series);
    }

    /**
     * Converts existing data points in the series between Celsius and Fahrenheit.
     *
     * @param series The series to convert.
     */
    private void convertExistingDataPoints(XYChart.Series<Number, Number> series) {
        for (XYChart.Data<Number, Number> data : series.getData()) {
            double currentValue = data.getYValue().doubleValue();
            double convertedValue;

            if (isCelsius) {
                // Convert from Fahrenheit to Celsius
                convertedValue = fahrenheitToCelsius(currentValue);
            } else {
                // Convert from Celsius to Fahrenheit
                convertedValue = celsiusToFahrenheit(currentValue);
            }

            // Update the Y-value of the data point
            data.setYValue(convertedValue);
        }
    }

    /**
     * Updates the charts and labels with new data points for each sensor.
     *
     * @param sensor1Value The value of sensor 1.
     * @param sensor2Value The value of sensor 2.
     */
    private void updateChartsAndLabels(double sensor1Value, double sensor2Value) {
        // Check for disconnected sensor values (-127°C) and default values (85°C)
        if (sensor1Value == -127.00 || sensor1Value == 85.00) {
            sensor1Label.setText("Sensor 1: Unplugged Sensor");
            for (XYChart.Data<Number, Number> data : sensor1Series.getData()) {
                data.setXValue(data.getXValue().intValue() - 1);
            }
            sensor1Series.getData().add(new XYChart.Data<>(0, sensor1Value));
            sensor1Series.getData().removeIf(data -> data.getXValue().intValue() < -300);
        } else {
            if (!isCelsius) {
                sensor1Value = celsiusToFahrenheit(sensor1Value);
            }
            sensor1Label.setText(String.format("Sensor 1: %.2f %s", sensor1Value, isCelsius ? "°C" : "°F"));
            // Shift the data to simulate scrolling graph behavior by decrementing x-values
            for (XYChart.Data<Number, Number> data : sensor1Series.getData()) {
                data.setXValue(data.getXValue().intValue() - 1);
            }
            sensor1Series.getData().add(new XYChart.Data<>(0, sensor1Value));
            sensor1Series.getData().removeIf(data -> data.getXValue().intValue() < -300);
        }

        if (sensor2Value == -127.00 || sensor2Value == 85.00) {
            sensor2Label.setText("Sensor 2: Unplugged Sensor");
            for (XYChart.Data<Number, Number> data : sensor2Series.getData()) {
                data.setXValue(data.getXValue().intValue() - 1);
            }
            sensor2Series.getData().add(new XYChart.Data<>(0, sensor2Value));
            sensor2Series.getData().removeIf(data -> data.getXValue().intValue() < -300);
        } else {
            if (!isCelsius) {
                sensor2Value = celsiusToFahrenheit(sensor2Value);
            }
            sensor2Label.setText(String.format("Sensor 2: %.2f %s", sensor2Value, isCelsius ? "°C" : "°F"));
            for (XYChart.Data<Number, Number> data : sensor2Series.getData()) {
                data.setXValue(data.getXValue().intValue() - 1);
            }
            sensor2Series.getData().add(new XYChart.Data<>(0, sensor2Value));
            sensor2Series.getData().removeIf(data -> data.getXValue().intValue() < -300);
        }

        // Increment the time variable to keep track of seconds elapsed
        time++;
    }

    /**
     * Converts Celsius to Fahrenheit.
     *
     * @param celsius The temperature in Celsius.
     * @return The temperature in Fahrenheit.
     */
    private double celsiusToFahrenheit(double celsius) {
        return (celsius * 9 / 5) + 32;
    }

    /**
     * Converts Fahrenheit to Celsius.
     *
     * @param fahrenheit The temperature in Fahrenheit.
     * @return The temperature in Celsius.
     */
    private double fahrenheitToCelsius(double fahrenheit) {
        return (fahrenheit - 32) * 5 / 9;
    }

    /**
     * Sends a command to the ESP32 to control sensor display.
     *
     * @param command The command to send (e.g., "toggleSensor1").
     */
    private void sendCommand(String command) {
        try (Socket socket = new Socket(ESP32_IP, COMMAND_PORT);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true)) {
            out.println(command);
            System.out.println("Sent command: " + command);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
