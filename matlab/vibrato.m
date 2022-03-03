filePath = '/Users/shanjiang/Documents/GitHub/2022-MUSI6106/bin/Debug/oboe_c6.wav';
[x, sampleRate] = audioread(filePath);
x = x(:,1);
N = length(x);
t = (0:N-1)/sampleRate;

% function y = vibrato(y, SAMPLERATE, Modfreq, Width)
SAMPLERATE = sampleRate;
Modfreq = 10;
Width = 0.001;
ya_alt=0;
Delay=Width; % basic delay of input sample in sec
DELAY=round(Delay*SAMPLERATE); % basic delay in # samples
WIDTH=round(Width*SAMPLERATE); % modulation width in # samples
if WIDTH>DELAY 
  error('delay greater than basic delay !!!');
  return;
end
MODFREQ=Modfreq/SAMPLERATE; % modulation frequency in # samples
LEN=length(x);        % # of samples in WAV-file
L=2+DELAY+WIDTH*2;    % length of the entire delay  
Delayline=zeros(L,1); % memory allocation for delay
y=zeros(size(x));     % memory allocation for output vector
for n=1:(LEN-1)
   M=MODFREQ;
   MOD=sin(M*2*pi*n);
   TAP=1+DELAY+WIDTH*MOD;
   i=floor(TAP);
   frac=TAP-i;
   Delayline=[x(n);Delayline(1:L-1)]; 
   %---Linear Interpolation-----------------------------
   y(n,1)=Delayline(i+1)*frac+Delayline(i)*(1-frac);
   %---Allpass Interpolation------------------------------
   %y(n,1)=(Delayline(i+1)+(1-frac)*Delayline(i)-(1-frac)*ya_alt);  
   %ya_alt=ya(n,1);
   %---Spline Interpolation-------------------------------
   %y(n,1)=Delayline(i+1)*frac^3/6
   %....+Delayline(i)*((1+frac)^3-4*frac^3)/6
   %....+Delayline(i-1)*((2-frac)^3-4*(1-frac)^3)/6
   %....+Delayline(i-2)*(1-frac)^3/6; 
   %3rd-order Spline Interpolation
end 

filePath_test = '/Users/shanjiang/Documents/GitHub/2022-MUSI6106/bin/Debug/oboe_c6_out.wav';
[x_test, sampleRate] = audioread(filePath_test);
x_test = x_test(:,1);

compare = y - x_test;

% amplitude graph
plot(t, compare);
grid on;
set(gca, 'FontName', 'Time New Roman', 'FontSize', 12);
xlabel('Time (s)');
ylabel('Ampl');
title('Processed Audio Diff Between Matlab and C++');

% w = hanning(N, 'periodic');
% periodogram(y, w, N, sampleRate, 'onesided');