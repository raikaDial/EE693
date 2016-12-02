% EE693 Fall 2016
% Team Project: General Purpose Bio-Monitoring System
% This program parses log files and plots the sensor data.

clc;
clear;
close all;

% How many samples per sensor per block
ECG_BLOCKSIZE = 1000;
EMG_BLOCKSIZE = 1000;
ADXL_BLOCKSIZE = 3*1000;
SPO2_BLOCKSIZE = 40;
FLAG_BYTES = 0;
BLOCKSIZE = 4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE ...
    + 2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE+FLAG_BYTES;

% Open the log file
[fileName, pathName] = uigetfile('*.bin', 'Select Log File');
if not(fileName)
    error('No file selected.')  
elseif isempty(regexp(fileName, '.bin$', 'once'))
    error('Invalid file selected.');
end

fileID = fopen([pathName, fileName]);
rawData = fread(fileID, 'uint8');
% magicNums = char(rawData(1:5))';
% 
% if not(strcmp(magicNums, 'EE693'))
%     error('Invalid file selected.');
% end
%rawData = rawData(6:end);

numBlocks = floor(length(rawData)/BLOCKSIZE); % Only process whole blocks
if not(numBlocks)
    error('Not enough data in file.');
end

% Pre-allocate buffers
ecgData = zeros(1, ECG_BLOCKSIZE*numBlocks);
emgData = zeros(1, EMG_BLOCKSIZE*numBlocks);
adxlData = zeros(1, ADXL_BLOCKSIZE*numBlocks);
spo2Data = zeros(1, SPO2_BLOCKSIZE*numBlocks);
flagData = zeros(1, FLAG_BYTES*numBlocks);

% Parse the raw data
for n = 0:numBlocks-1
    % Parse ECG Data: 4 bytes -> float
    for i=1:ECG_BLOCKSIZE
        ecgData(i+ECG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( ...
                        rawData(4*i-3), ...
                        bitshift(rawData(4*i-2), 8, 'uint32'), ...
                        'uint32' ...
                    )), ...
                    uint32(bitor( ...
                        bitshift(rawData(4*i-1), 16, 'uint32'), ...
                        bitshift(rawData(4*i), 24, 'uint32'), ...
                        'uint32' ...
                    )), ...
                    'uint32' ...
                )), ...
                'single'...
            );   
    end
    % Remove bytes that have already been parsed from the raw data array.
    %     This isn't the most efficient way to parse the data, but this
    %     simplifies the logic quite a bit.
    rawData = rawData(4*ECG_BLOCKSIZE+1:length(rawData));
    
    % Parse EMG Data: 2 bytes -> uint16
    for i=1:EMG_BLOCKSIZE
        emgData(i+EMG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( ...
                        rawData(4*i-3), ...
                        bitshift(rawData(4*i-2), 8, 'uint32'), ...
                        'uint32' ...
                    )), ...
                    uint32(bitor( ...
                        bitshift(rawData(4*i-1), 16, 'uint32'), ...
                        bitshift(rawData(4*i), 24, 'uint32'), ...
                        'uint32' ...
                    )), ...
                    'uint32' ...
                )), ...
                'single'...
            );   
    end
    rawData = rawData(4*EMG_BLOCKSIZE+1:length(rawData));
    
    % Parse ADXL Data: 6 bytes -> 3*int16 (x, y, z ft/s^2)
    for i=1:ADXL_BLOCKSIZE
        adxlData(i+ADXL_BLOCKSIZE*n) = ...
            typecast(uint16(bitor(rawData(2*i-1), ...
                bitshift(rawData(2*i), 8, 'uint16'), 'uint16')), ...
            'int16');
    end
    rawData = rawData(2*ADXL_BLOCKSIZE+1:length(rawData));
    
    % Parse SPO2 Data: 1 byte -> uint8 (%SPO2)
    for i=1:SPO2_BLOCKSIZE
        spo2Data(i+SPO2_BLOCKSIZE*n) = rawData(i);
    end
    rawData = rawData(SPO2_BLOCKSIZE+1:length(rawData));
    
    % Parse Flag Byte
%     for i=1:FLAG_BYTES
%         flagData(n+i) = rawData(i);
%     end
    rawData = rawData(FLAG_BYTES+1:length(rawData));
end
adxlData = reshape(adxlData,3,[]);

fs = 250;
fs_spo2 = 10;
t = 0:1/fs:(length(ecgData)-1)/fs;
t_spo2 = 0:1/fs_spo2:(length(spo2Data)-1)/fs_spo2;
t_accel = 0:1/fs:(length(adxlData)-1)/fs;

for n=1:60
    ecgData(n) = ecgData(60);
    emgData(n) = emgData(60);
end

% Plot ECG Data
figure('Name', 'ECG Data')
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')

% Plot EMG Data
figure('Name', 'EMG Data')
plot(t, emgData);
title('EMG Data')
xlabel('Time (s)')

% Plot Accelerometer Data

maxadxl_scale=max([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
maxadxl_scale=max(maxadxl_scale(:,:,:));

minadxl_scale=min([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
minadxl_scale=min(minadxl_scale(:,:,:));

figure('Name', 'Accel_')
subplot(3,1,1)
plot(t_accel, adxlData(1,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(3,1,2)
plot(t_accel, adxlData(2,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer y-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(3,1,3)
plot(t_accel, adxlData(3,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer z-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
saveas(gcf,[pathName,'_ADXL'],'png')

% Plot SPO2 Data
figure('Name', 'SPO2')
plot(t_spo2, spo2Data)
title('SPO2')
xlabel('Time (s)')
ylabel('%')
