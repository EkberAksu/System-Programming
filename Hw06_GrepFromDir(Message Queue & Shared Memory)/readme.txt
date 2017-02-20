kod valgrind de test edildiginde ,bazen fopen da memory leak gosteriyor.
oysa fopen dan sonra gereken yerde fclose() fonsiyonuyla katilmisti. Internetten arastirdigimda 
bu fopen fonksiyonun prolemi oldugunu ogrendim, fopen fonksiyonu kendisi malloc
kullaniyormus, ve bizde onu free edemedigimizden bazi durumlarda bu tip sorunlar ortaya cikiyormus.

