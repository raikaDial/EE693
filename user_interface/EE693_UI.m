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
BLOCKSIZE = 2*ECG_BLOCKSIZE+2*EMG_BLOCKSIZE ...
    + 2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE;

% Open the log file
[fileName, pathName] = uigetfile('*.bin', 'Select Log File');
if not(fileName)
    error('No file selected.')  
elseif isempty(regexp(fileName, '.bin$', 'once'))
    error('Invalid file selected.');
end

fileID = fopen([pathName, fileName]);
rawData = fread(fileID, 'uint8');
fileName = fileName(1:end-4);
fileDescrip = fileName(1:end-9);
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

for n = 0:numBlocks-1
    for i=1:ECG_BLOCKSIZE
        ecgData(i+ECG_BLOCKSIZE*n) = bitor(rawData(2*i+BLOCKSIZE*n-1), ...
            bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16');
    end
    for i=ECG_BLOCKSIZE+1:ECG_BLOCKSIZE+EMG_BLOCKSIZE
       emgData(i-ECG_BLOCKSIZE+EMG_BLOCKSIZE*n) = bitor( ...
           rawData(2*i+BLOCKSIZE*n-1), ...
           bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16'); 
    end
    for i=ECG_BLOCKSIZE+EMG_BLOCKSIZE+1:ECG_BLOCKSIZE+...
            EMG_BLOCKSIZE+ADXL_BLOCKSIZE
        adxlData(i-(ECG_BLOCKSIZE+EMG_BLOCKSIZE)+ADXL_BLOCKSIZE*n) = ...
            typecast(uint16(bitor(rawData(2*i+BLOCKSIZE*n-1), ...
            bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16')),...
            'int16');
    end
    for i=2*(ECG_BLOCKSIZE+EMG_BLOCKSIZE+ADXL_BLOCKSIZE)+1:BLOCKSIZE
        spo2Data(i+SPO2_BLOCKSIZE*n-2*(ECG_BLOCKSIZE+EMG_BLOCKSIZE+...
            ADXL_BLOCKSIZE)) = rawData(i+BLOCKSIZE*n);
    end
end
adxlData = reshape(adxlData,3,[]);

fs = 250;
fs_spo2 = 10;
t = 0:1/fs:(length(ecgData)-1)/fs;
t_spo2 = 0:1/fs_spo2:(length(spo2Data)-1)/fs_spo2;
t_accel = 0:1/fs:(length(adxlData)-1)/fs;
% Plot ECG Data
figure('Name', ['ECG_',fileDescrip])
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')
saveas(gcf,[pathName,fileDescrip,'_ECG'],'png')
% % Plot EMG Data
% figure('Name', 'EMG Data')
% plot(t, emgData);
% title('EMG Data')
% xlabel('Time (s)')


% Plot Accelerometer Data

maxadxl_scale=max([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
maxadxl_scale=max(maxadxl_scale(:,:,:));

minadxl_scale=min([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
minadxl_scale=min(minadxl_scale(:,:,:));

figure('Name', ['Accel_',fileDescrip])
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
saveas(gcf,[pathName,fileDescrip,'_ADXL'],'png')

% % Plot SPO2 Data
% figure('Name', 'SPO2')
% plot(t_spo2, spo2Data)
% title('SPO2')
% xlabel('Time (s)')
% ylabel('%')
