% EE693 Fall 2016
% Team Project: General Purpose Bio-Monitoring System
% This program parses log files and plots the sensor data.

clc;
clear;
close all;

% How many samples per sensor per block
pointer=0;
ECG_BLOCKSIZE = 1000;
EMG_BLOCKSIZE = 1000;
ADXL_BLOCKSIZE = 3*1000;
SPO2_BLOCKSIZE = 40;
FLAG_BYTES = 1000;
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
flaglocation = zeros(1, FLAG_BYTES*numBlocks);

% Parse the raw data
for n = 0:numBlocks-1
    % Parse ECG Data: 4 bytes -> float
    start=(BLOCKSIZE*n)
    
    for i=1:ECG_BLOCKSIZE
%         Datatemp00=dec2bin(bitshift(rawData(i), 24, 'uint32'),32)
%         Datatemp11=dec2bin(bitshift(rawData(i-1), 16, 'uint32'),32)
%         Datatemp22=dec2bin(bitshift(rawData(i-2), 8, 'uint32'),32)
%         Datatemp33=dec2bin(rawData(i-3),32)
        
        Datatemp3=rawData(i*4-3+(BLOCKSIZE*n));
        Datatemp2=bitshift(rawData(i*4-2+(BLOCKSIZE*n)), 8, 'uint32');
        Datatemp1=bitshift(rawData(i*4-1+(BLOCKSIZE*n)), 16, 'uint32');
        Datatemp0=bitshift(rawData(i*4+(BLOCKSIZE*n)), 24, 'uint32');
%         or10=dec2bin(uint32(bitor( Datatemp1,Datatemp0,'uint32')),32)
%         or23=dec2bin(uint32(bitor( Datatemp2,Datatemp3,'uint32')),32)
        
        ecgData(i+ECG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( Datatemp3,Datatemp2,'uint32')), ...
                    uint32(bitor( Datatemp1,Datatemp0,'uint32')), ...
                'uint32' )),...
             'single');  
%         total= dec2bin(ecgData(i+ECG_BLOCKSIZE*n),32) 
pointer=4*ECG_BLOCKSIZE;
    end
    % Parse EMG Data: 2 bytes -> uint16
    for i=1:EMG_BLOCKSIZE
        Datatemp3=rawData(i*4-3+(BLOCKSIZE*n)+pointer);
        Datatemp2=bitshift(rawData(i*4-2+(BLOCKSIZE*n)+pointer), 8, 'uint32');
        Datatemp1=bitshift(rawData(i*4-1+(BLOCKSIZE*n)+pointer), 16, 'uint32');
        Datatemp0=bitshift(rawData(i*4+(BLOCKSIZE*n)+pointer), 24, 'uint32');
        
        emgData(i+ECG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( Datatemp3,Datatemp2,'uint32')), ...
                    uint32(bitor( Datatemp1,Datatemp0,'uint32')), ...
                'uint32' )),...
             'single');   
    end
    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE;
    % Parse ADXL Data: 6 bytes -> 3*int16 (x, y, z ft/s^2)
    for i=1:ADXL_BLOCKSIZE
        adxlData(i+ADXL_BLOCKSIZE*n) = ...
            typecast(uint16(bitor(rawData(2*i-1+(BLOCKSIZE*n)+ pointer), ...
                bitshift(rawData(2*i+(BLOCKSIZE*n)+pointer), 8, 'uint16'),...
                'uint16')), ...
                  'int16');
    end

    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE;
    % Parse SPO2 Data: 1 byte -> uint8 (%SPO2)
    for i=1:SPO2_BLOCKSIZE
        spo2Data(i+SPO2_BLOCKSIZE*n) = rawData(i+(BLOCKSIZE*n)+ pointer);
    end
     pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE;
        % Parse Flag Byte
    for i=1:FLAG_BYTES
        position=i+(BLOCKSIZE*n)+pointer;
        flagData(FLAG_BYTES*n+i) = rawData(i+(BLOCKSIZE*n)+pointer);
    end
    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE+FLAG_BYTES;
end
adxlData = reshape(adxlData,3,[]);

fs = 250;
fs_spo2 = 10;
t = 0:1/fs:(length(ecgData)-1)/fs;
t_spo2 = 0:1/fs_spo2:(length(spo2Data)-1)/fs_spo2;
t_accel = 0:1/fs:(length(adxlData)-1)/fs;

for n=1:100
    ecgData(n) = ecgData(100);
    emgData(n) = emgData(100);
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
subplot(4,1,1)
plot(t_accel, adxlData(1,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(4,1,2)
plot(t_accel, adxlData(2,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer y-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(4,1,3)
plot(t_accel, adxlData(3,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer z-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(4,1,4)
plot(t_accel,flagData(1,:))
axis([0,max(t_accel),0,1])
title('FlagData')
xlabel('Time (s)')
ylabel('ft^2/s')
saveas(gcf,[pathName,'_ADXL'],'png')

% Plot SPO2 Data
figure('Name', 'SPO2')
plot(t_spo2, spo2Data)
title('SPO2')
xlabel('Time (s)')
ylabel('%')