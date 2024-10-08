function MatlabWebServer()
    import java.net.ServerSocket
    import java.net.Socket
    import java.io.*

    % Specify the port to listen on
    port = 8080;
    serverSocket = ServerSocket(port);
    disp(['MATLAB HTTP server is running on port ' num2str(port)]);

    % Keep the server running indefinitely
    while true
        % Wait for a client to connect
        clientSocket = serverSocket.accept();
        disp('Client connected.');

        % Handle the client in a separate function
        handleClient(clientSocket);
    end

    % Close the server socket when done (this line is never reached)
    % serverSocket.close();
end

function handleClient(clientSocket)
    import java.io.*

    try
        % Get input and output streams from the socket
        inputStream = clientSocket.getInputStream();
        dataInputStream = BufferedReader(InputStreamReader(inputStream));

        outputStream = clientSocket.getOutputStream();
        dataOutputStream = PrintWriter(OutputStreamWriter(outputStream, 'UTF-8'), true);

        % Read the HTTP request line
        requestLine = char(dataInputStream.readLine());
        disp(['Received request: ' requestLine]);

        % Read and process HTTP headers
        headers = '';
        contentLength = 0;
        while true
            line = char(dataInputStream.readLine());
            if isempty(line)
                break;  % Empty line indicates the end of headers
            end
            headers = [headers line newline];

            % Check for Content-Length header
            if startsWith(line, 'Content-Length:')
                contentLength = str2double(strtrim(line(16:end)));
            end
        end

        % Read the request body (if any)
        body = '';
        if contentLength > 0
            charArray = zeros(1, contentLength, 'uint8');
            for i = 1:contentLength
                charArray(i) = dataInputStream.read();
            end
            body = native2unicode(charArray, 'UTF-8');
        end

        % Check if the request is a POST request to '/receive_data'
        if startsWith(requestLine, 'POST') && contains(requestLine, '/receive_data')
            % Parse the JSON data from the request body
            receivedData = jsondecode(body);
            dataValues = receivedData.data;  % Extract the 'data' field
            disp('Data received from ESP32:');
            disp(dataValues);

            % Generate the graph
            fig = figure('Visible', 'off');
            plot(dataValues, 'o-');
            title('Data Received from ESP32');
            xlabel('Index');
            ylabel('Value');

            % Save the graph as a PNG image
            graphFile = 'graph.png';
            saveas(fig, graphFile);
            close(fig);

            % Read the image file and encode it in Base64
            fid = fopen(graphFile, 'r');
            imgData = fread(fid, '*uint8');
            fclose(fid);
            encodedGraph = matlab.net.base64encode(imgData);

            % Create a JSON response containing the Base64-encoded image
            responseStruct = struct('image', encodedGraph);
            jsonResponse = jsonencode(responseStruct);

            % Prepare the HTTP response
            responseLines = {
                'HTTP/1.1 200 OK'
                'Content-Type: application/json'
                ['Content-Length: ' num2str(length(jsonResponse))]
                ''  % Blank line to indicate end of headers
                jsonResponse
            };
            response = strjoin(responseLines, newline);

            % Send the response
            dataOutputStream.print(response);
            dataOutputStream.flush();
            disp('Response sent to ESP32.');
        else
            % Handle unsupported requests
            responseLines = {
                'HTTP/1.1 405 Method Not Allowed'
                'Content-Length: 0'
                ''  % Blank line to indicate end of headers
                ''
            };
            response = strjoin(responseLines, newline);
            dataOutputStream.print(response);
            dataOutputStream.flush();
            disp('Unsupported request received.');
        end

        % Close the client socket
        clientSocket.close();
    catch ME
        disp(['Error handling client: ' ME.message]);
        clientSocket.close();
    end
% After saving the graph as 'graph.png'
fileInfo = dir(graphFile);
disp(['Size of generated graph.png: ' num2str(fileInfo.bytes) ' bytes']);
disp(['Length of Base64-encoded image: ' num2str(length(encodedGraph))]);
disp(['JSON response being sent to ESP32: ' jsonResponse]);



end
