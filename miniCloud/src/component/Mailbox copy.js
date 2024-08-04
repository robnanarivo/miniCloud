// import React from 'react';
// import MenuBar from './MenuBar';

// import { Card, Col, Row } from 'antd';
// const { Meta } = Card;
// let sample = [
//   {
//     id: 1,
//     name: "Inbox",
//     emails: [
//       {
//         id: 1,
//         from: "joe@tryolabs.com",
//         to: "fernando@tryolabs.com",
//         subject: "Meeting",
//         body: "hi",
//       },
//       {
//         id: 2,
//         from: "newsbot@tryolabs.com",
//         to: "fernando@tryolabs.com",
//         subject: "News Digest",
//         body: "<h1>Intro to React</h1> <img src='https://raw.githubusercontent.com/wiki/facebook/react/react-logo-1000-transparent.png' width=300/)>",
//       },
//     ],
//   },
//   {
//     id: 2,
//     name: "Spam",
//     emails: [
//       {
//         id: 3,
//         from: "nigerian.prince@gmail.com",
//         to: "fernando@tryolabs.com",
//         subject: "Obivous 419 scam",
//         body: "You've won the prize!!!1!1!!!",
//       },
//     ],
//   },
// ]
// class Email extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }
//   }
//   render() {
//     return (
//       <div className="email">
//         <dl className="meta dl-horizontal">
//           <dt>From</dt>
//           <dd>{this.props.from}</dd>

//           <dt>To</dt>
//           <dd>{this.props.to}</dd>

//           <dt>Subject</dt>
//           <dd>{this.props.subject}</dd>
//         </dl>
//         <div
//           className="body"
//           dangerouslySetInnerHTML={{ __html: this.props.body }}
//         />
//       </div>
//     )
//   }
// }
// class EmailList extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }
//   }
//     render() {
//     let email_list = this.props.emails.map(
//       function (mail) {
//         return (
//           <EmailListItem
//             key={mail.id}
//             from={mail.from}
//             to={mail.to}
//             subject={mail.subject}
//             on_click={this.props.onSelectEmail.bind(null, mail.id)}
//           />
//         )
//       }.bind(this)
//     )

//     return (
//       <table className="email-list table table-striped table-condensed">
//         <thead>
//           <tr>
//             <th>Subject</th>
//             <th>From</th>
//             <th>To</th>
//           </tr>
//         </thead>
//         <tbody>{email_list}</tbody>
//       </table>
//     )
//   }
// }
// class EmailListItem extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }
//   }
//   render() {
//     return (
//       <tr onClick={this.props.on_click.bind(null)}>
//         <td>{this.props.subject}</td>
//         <td>{this.props.from}</td>
//         <td>{this.props.to}</td>
//       </tr>
//     )
//   }
// }
// class NoneSelected extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }
//   }
//   render() {
//     return (
//       <div className="none-selected alert alert-warning" role="alert">
//         <span>No {this.props.text} selected.</span>
//       </div>
//     )
//   }
// }

// class Mailbox extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }

//     this.getInitialState = this.getInitialState.bind(this)
//     this.handleSelectEmail = this.handleSelectEmail.bind(this)
//   }

//   getInitialState () {
//     return { email_id: null }
//   }

//   handleSelectEmail (id) {
//     this.setState({ email_id: id })
//   }

//   render() {
//     let email_id = this.state.email_id
//     if (email_id) {
//       let mail = this.props.emails.filter(function (mail) {
//         return mail.id == email_id
//       })[0]
//       this.setState({selected_email: <Email
//         id={mail.id}
//         from={mail.from}
//         to={mail.to}
//         subject={mail.subject}
//         body={mail.body}
//       />})
//     } else {
//       this.setState({selected_email: <NoneSelected text="email" />})
//     }

//     return (
//       <div>
//         <EmailList
//           emails={this.props.emails}
//           onSelectEmail={this.handleSelectEmail}
//         />
//         <div className="email-viewer">{this.state.selected_email}</div>
//       </div>
//     )
//   }
// }

// class MailboxList extends React.Component {
//   constructor(props) {
//     super(props)
//     this.state = {
//     }
//     this.onClickMailbox = this.onClickMailbox.bind(this)
//   }
//   onClickMailbox(id){
//     this.props.onSelectMailbox(id);
//   }

//   render() {
//     let mailbox_list = this.props.mailboxes.map(
//       function (mailbox) {
//         return (
//           <li
//             className="list-group-item"
//             key={mailbox.id}
//             onClick={()=>{this.onClickMailbox(mailbox.id)}}
//           >
//             <span className="badge">{mailbox.emails.length}</span>
//             {mailbox.name}
//           </li>
//         )
//       }.bind(this)
//     )

//     return (
//       <div className="col-md-2">
//         <ul className="mailboxes list-group">{mailbox_list}</ul>
//       </div>
//     )
//   }
// }
// class MainMailbox extends React.Component {
//   constructor(props) {
//       super(props)
//       this.state = {
//         selected_mailbox: null
//       }

//       this.getInitialState = this.getInitialState.bind(this)
//       this.handleSelectMailbox = this.handleSelectMailbox.bind(this)
//   }

  

//   getInitialState() {
//     return { mailbox_id: null }
//   }
//   handleSelectMailbox(id) {
//     console.log("@@@@@click@@", id)
//     this.setState({ mailbox_id: id })
//   }
//   componentDidUpdate() {
//     let mailbox_id = this.state.mailbox_id
//     if (mailbox_id) {
//       let mailbox = sample.filter(function (mailbox) {
//         return mailbox.id == mailbox_id
//       })[0]
//       this.setState({ selected_mailbox: <Mailbox key={mailbox.id} emails={mailbox.emails} /> })
//     } else {
//       this.setState({ selected_mailbox: <NoneSelected text="mailbox" /> })
//     }
//   }

//   render() {


//     return (
//       <div className="app row">
//         <MailboxList
//           mailboxes={sample}
//           onSelectMailbox={this.handleSelectMailbox}
//         />
//         <div className="mailbox col-md-10">
//           <div className="panel panel-default">
//             <div className="panel-body">{this.state.selected_mailbox}</div>
//           </div>
//         </div>
//       </div>
//     )
//   }
// }
// export default MainMailbox;