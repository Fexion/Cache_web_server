# Cache_web_server
##Смирнов Александр Олегович, 151, Кэширующий веб-сервер.

##Постанавка Задачи
Написание кэширующего веб-сервера.
##Описание
Кэширующий веб-сервер представляет собой выделенный сетевой сервер или службу, действующую в качестве сервера который позволяет сохранять локально веб страницы или другой интернет контент. Размещая ранее запрошенную информацию в местах временного хранения, кэш-сервер ускоряет доступ к данным и снижает нагрузку на интернет соединение. Серверы кэширования также позволяют пользователю получать доступ к данным в автономном режиме.

##Существующие решения: 
- Nginx
- Apache HTTP Server


В данном проекте используется только низкоуровневые API систем Linux и FreeBSD для
отработки практических навыков по курсу АКОС.
Использование механизма epoll/kqueue.


##План работы:
- [ ] Реализация веб сервера, умеющего выдавать статические веб страницы.
- [ ] Сервер умеет перенаправлять запрос другому серверу и передавать обратно ответ
- [ ] Умение обрабатывать запросы в несколько потоков с использованием thread 
