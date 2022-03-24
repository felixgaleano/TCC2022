phical = -2:.1:2;
fa=834.93;
f=60;
dt=1/fa;

A=(1-phical)*cos(2*pi*f*dt)+phical;
B=(1-phical)*sin(2*pi*f*dt);
fi=atan(B./A)*180/pi;
natraso=atan(B./A)/(2*pi*60)/dt;
M=sqrt(A.^2+B.^2);

subplot(2,1,1)
plot(phical,natraso,'linewidth',2);
xlabel('Constante de calibracao PHI_c_a_l','fontsize',8)
ylabel('Atraso [amostras]','fontsize',8)
grid on

subplot(2,1,2)
plot(phical,M,'linewidth',2);
xlabel('Constante de calibracao PHI_c_a_l','fontsize',8)
ylabel('Deformacao na amplitude [M]','fontsize',8)
grid on