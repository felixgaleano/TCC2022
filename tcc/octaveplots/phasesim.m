PHASECAL = 2;
fa=834.93;
f=60;
dt=1/fa;

A=(1-PHASECAL)*cos(2*pi*f*dt)+PHASECAL;
B=(1-PHASECAL)*sin(2*pi*f*dt);
fi=atan(B/A);
M=sqrt(A^2+B^2);

n=0:1:20;
t=n*dt;

V=sin(2*f*pi*t);
lastFilteredV=V(1:end-1);
filteredV=V(2:end);

phaseShiftedV = lastFilteredV + PHASECAL * (filteredV - lastFilteredV);

tatraso=fi/(2*pi*f);
Vatrasada=sin(2*pi*f*(t-tatraso));

figure; hold on;
plot(n,V,'linewidth',1);
plot(n,Vatrasada,'g','linewidth',1);
plot(n(2:end),phaseShiftedV./M,'r*','linewidth',1);
grid on

M1="Tensao amostrada";
M2="Tesao calculada utilizando-se a teoria";
M3="Tensao amostrada defasada pela expressão da biblioteca";

legend(M1, M2, M3);

xlabel('Amostras [n] ','fontsize',12)
ylabel('Tensao [V]','fontsize',12)
