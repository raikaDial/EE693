% x1= importdata('logan20lboldr.txt');
% y = x1(:);
% subplot(311);
% plot(y);
% xlabel('Sample number');
% ylabel('Envelop EMG signal');
% grid on;

% x= importdata('logan20lbold.txt');
% y1 = x(:);
% 
% subplot(312);plot(y1);
% xlabel('Sample number');
% ylabel('Noisy EMG signal');
% grid on;
y=emgData;
%y=emgshort;
subplot(211);
plot(y);
xlabel('Sample number');
y2=detrend(y);
rec_y=abs(y2);
%y3=abs(rec_y-mean(rec_y));
%figure(9);plot(rec_y);
%xlabel('Sample number');
%ylabel('Rectified EMG signal');
fnorm= [20/250];
[b,a]=butter(2,fnorm,'low');
filter_y=filtfilt(b,a,rec_y);
subplot(212);
plot(filter_y);
xlabel('Sample number');
ylabel('Low Pass Filtered EMG signal');
grid on;
y1=filter_y;
N = length(rec_y);% find the length of the data per second
ls = size(rec_y); %% size
%f = 1/N;% find the sampling rate or frequency
fs = 250;
T = 1/fs; % period between each sample
t1 = (0 : N-1) *T;%t = (0:1:length(y1)-1)/fs; % sampling period
figure(10);
subplot(211);plot(t1, y1);
grid on;

 for ii= 1:2
     t1(end)=ii;
     y1(end)= y1(t1(end));
     thy1= abs(mean(y1(end)));
 end
 th = thy1;

f1=0; f2=0;
L = length(y1);
out = 0;
for onset = 1:L
    if y1(onset) > th
       
        f1= y1-th;
       f1(f1<0)=0;
        
        if f1<0; f1=0; out= true;
            
        end
        if y1(onset)<th
         f2=0;
        out =false;
        end 
   else
        out = true;
    end
end

f=f1+f2;
subplot(212); plot(t1,f);
grid on;
%tp=.2;
%emg_m = conv(f ,ones(1 ,round(tp*fs))/round(tp*fs));
%figure(72);plot(emg_m);

%RMS of the signal
rms_y1 = sqrt(mean(f.^2));
msgbox(strcat('RMS of Active EMG signal is = ',mat2str(rms_y1), ''));
rms_emg = rms (f);

%RMS of the signal
%rms_env = sqrt(mean(filter_y.^2));
%msgbox(strcat('RMS of EMG signal is = ',mat2str(rms_env), ''));
%rms_env2 = rms (filter_y);

%AVR of the signal
arv_y1 = abs(mean(f));
msgbox(strcat('ARV of ActiveEMG signal is = ',mat2str(arv_y1), ''));

%AVR of the signal
%arv_env1 = abs(mean(y1));
%msgbox(strcat('ARV of EMG signal is = ',mat2str(arv_env1), ''));

k=length(t1);

for i= 1:2500:k-2500
%  for j=i:i+2499
j=i:i+2499;
   if max(f(j))<arv_y1
     disp('Simple R calculation')
    else
        disp('Pan-Tompkins method applied')
    end
 end
% end
 
