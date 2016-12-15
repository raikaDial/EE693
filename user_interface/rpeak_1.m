function hr = rpeak_1(s2)
% i_1=1;
% h=[];
% h(1)=0;
% h_rate=0;
% h_rate_graph=zeros(1,length(s));
% % h_rate_graph(1)=0;
% 
% for i=2:length(s)-1
%     if i-i_1>50
%     if (s(i)>s(i-1)) && (s(i)>s(i+1)) && (s(i)>0.06)
%         h_rate=h_rate+1;
%         h(i)=fix(h_rate*60*250/i);
%         i_1=i;
%     end
%     end
%     h_rate_graph(i)=h(i_1);
% end

thr = max(s2(1:500))*.25;
% thr=0.06;
[A,P] = findpeaks(s2,'MINPEAKDISTANCE',.2*250,'MINPEAKHEIGHT',thr);

% plot(s1)
% hold on
% scatter(P-49,s_r(P-49))

hr=fix(250*60*length(A)/length(s2));