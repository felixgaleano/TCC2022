fa = 834.93;
dt=1/fa;
freq=60;
al=1/1024;
nsamples = 100;
samples = 0:1:nsamples;
t=samples*dt;
SampleV=2.5+0.9*cos(2*pi*freq*t);
offset=zeros(1,nsamples+1);
offset(1)=2.5;

for n=2:nsamples+1
offset(n)=offset(n-1)+al*(SampleV(n)-offset(n-1));
end

plot(samples,offset,'linewidth',1,'r');
xlabel('n [amostra]','fontsize',12)
ylabel('Offset [V]','fontsize',12)